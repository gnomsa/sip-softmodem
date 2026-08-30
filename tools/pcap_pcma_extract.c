#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0]<<8|p[1]);
}

static uint32_t be32(const uint8_t *p)
{
    return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|
           (uint32_t)p[2]<<8|p[3];
}

static uint32_t file32(const uint8_t *p,int little)
{
    if(little)
        return (uint32_t)p[0]|(uint32_t)p[1]<<8|
               (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
    return be32(p);
}

static size_t network_offset(const uint8_t *packet,size_t size,uint32_t link)
{
    if(link==101)return 0;             /* raw IPv4 */
    if(link==113)return size>=16&&be16(packet+14)==0x0800?16:SIZE_MAX;
    if(link==276)return size>=20&&be16(packet)==0x0800?20:SIZE_MAX;
    if(link==1){
        if(size<14)return SIZE_MAX;
        uint16_t type=be16(packet+12);size_t at=14;
        while((type==0x8100||type==0x88a8)&&size>=at+4){
            type=be16(packet+at+2);at+=4;
        }
        return type==0x0800?at:SIZE_MAX;
    }
    return SIZE_MAX;
}

static int extract_packet(const uint8_t *packet,size_t size,uint32_t link,
                          unsigned dst_port,FILE *out,uint16_t *last_seq,
                          uint32_t *last_timestamp,int *have_last,
                          unsigned long *packets,unsigned long *samples,
                          unsigned long *sequence_gaps,
                          unsigned long *timestamp_steps)
{
    size_t ip_at=network_offset(packet,size,link);
    if(ip_at==SIZE_MAX||size<ip_at+20)return 0;
    const uint8_t *ip=packet+ip_at;
    if((ip[0]>>4)!=4||ip[9]!=17)return 0;
    size_t ihl=(size_t)(ip[0]&15u)*4;
    if(ihl<20||size<ip_at+ihl+8)return 0;
    const uint8_t *udp=ip+ihl;
    if(be16(udp+2)!=dst_port)return 0;
    size_t udp_length=be16(udp+4);
    if(udp_length<20||ip_at+ihl+udp_length>size)return 0;
    const uint8_t *rtp=udp+8;size_t rtp_size=udp_length-8;
    if((rtp[0]>>6)!=2||(rtp[1]&0x7fu)!=8)return 0;
    size_t header=12u+4u*(rtp[0]&15u);
    if(header>rtp_size)return 0;
    if(rtp[0]&0x10u){
        if(header+4>rtp_size)return 0;
        header+=4u+4u*be16(rtp+header+2);
        if(header>rtp_size)return 0;
    }
    size_t payload=rtp_size-header;
    if(rtp[0]&0x20u){
        if(!payload||rtp[rtp_size-1]>payload)return 0;
        payload-=rtp[rtp_size-1];
    }
    uint16_t sequence=be16(rtp+2);uint32_t timestamp=be32(rtp+4);
    if(*have_last){
        uint16_t sequence_delta=(uint16_t)(sequence-*last_seq);
        uint32_t timestamp_delta=timestamp-*last_timestamp;
        if(sequence_delta!=1)*sequence_gaps+=(uint16_t)(sequence_delta-1);
        if(timestamp_delta!=160)*timestamp_steps+=1;
    }
    if(payload&&fwrite(rtp+header,1,payload,out)!=payload)return -1;
    *last_seq=sequence;*last_timestamp=timestamp;*have_last=1;
    *packets+=1;*samples+=payload;
    return 1;
}

int main(int argc,char **argv)
{
    if(argc!=4){
        fprintf(stderr,"usage: %s CAPTURE.pcap DST_PORT OUTPUT.pcma\n",argv[0]);
        return 2;
    }
    char *end=0;unsigned long requested=strtoul(argv[2],&end,10);
    if(!argv[2][0]||*end||requested>65535){
        fprintf(stderr,"invalid UDP destination port: %s\n",argv[2]);return 2;
    }
    FILE *in=fopen(argv[1],"rb");
    if(!in){fprintf(stderr,"%s: %s\n",argv[1],strerror(errno));return 1;}
    FILE *out=fopen(argv[3],"wb");
    if(!out){fprintf(stderr,"%s: %s\n",argv[3],strerror(errno));fclose(in);return 1;}
    uint8_t global[24];
    if(fread(global,1,sizeof global,in)!=sizeof global){
        fprintf(stderr,"short pcap global header\n");fclose(in);fclose(out);return 1;
    }
    int little;
    if(!memcmp(global,"\xd4\xc3\xb2\xa1",4)||
       !memcmp(global,"\x4d\x3c\xb2\xa1",4))little=1;
    else if(!memcmp(global,"\xa1\xb2\xc3\xd4",4)||
            !memcmp(global,"\xa1\xb2\x3c\x4d",4))little=0;
    else {fprintf(stderr,"input is not classic pcap; convert with editcap -F pcap\n");fclose(in);fclose(out);return 1;}
    uint32_t link=file32(global+20,little);
    if(link!=1&&link!=101&&link!=113&&link!=276){
        fprintf(stderr,"unsupported pcap link type %u\n",link);fclose(in);fclose(out);return 1;
    }
    uint8_t record[16],*packet=0;size_t capacity=0;
    uint16_t last_sequence=0;uint32_t last_timestamp=0;int have_last=0;
    unsigned long packets=0,samples=0,gaps=0,bad_steps=0;int status=0;
    while(fread(record,1,sizeof record,in)==sizeof record){
        uint32_t captured=file32(record+8,little);
        if(captured>16u*1024u*1024u){fprintf(stderr,"invalid captured length %u\n",captured);status=1;break;}
        if(captured>capacity){
            uint8_t *larger=realloc(packet,captured);
            if(!larger){fprintf(stderr,"out of memory\n");status=1;break;}
            packet=larger;capacity=captured;
        }
        if(fread(packet,1,captured,in)!=captured){fprintf(stderr,"short pcap record\n");status=1;break;}
        if(extract_packet(packet,captured,link,(unsigned)requested,out,
                          &last_sequence,&last_timestamp,&have_last,&packets,
                          &samples,&gaps,&bad_steps)<0){
            fprintf(stderr,"write failed: %s\n",strerror(errno));status=1;break;
        }
    }
    free(packet);
    if(ferror(in)){fprintf(stderr,"read failed: %s\n",strerror(errno));status=1;}
    if(fclose(out)!=0){fprintf(stderr,"close failed: %s\n",strerror(errno));status=1;}
    fclose(in);
    fprintf(stderr,"RTP PCMA packets=%lu samples=%lu sequence-gaps=%lu non-160-timestamps=%lu\n",
            packets,samples,gaps,bad_steps);
    return status||!packets;
}
