#include "v42_hdlc.h"
#include <string.h>

static uint16_t update(uint16_t fcs,uint8_t octet)
{
    fcs^=octet;
    for(int i=0;i<8;i++)fcs=(fcs&1)?(uint16_t)((fcs>>1)^0x8408):(uint16_t)(fcs>>1);
    return fcs;
}

uint16_t v42_hdlc_fcs(const uint8_t*d,size_t n)
{
    uint16_t f=0xffff;for(size_t i=0;i<n;i++)f=update(f,d[i]);return(uint16_t)~f;
}

static int put(uint8_t*out,size_t cap,size_t*at,int bit)
{if(*at>=cap)return-1;out[(*at)++]=(uint8_t)(bit&1);return 0;}
static int flag(uint8_t*out,size_t cap,size_t*at)
{for(int i=0;i<8;i++)if(put(out,cap,at,(0x7e>>i)&1)<0)return-1;return 0;}

size_t v42_hdlc_encode_raw(const uint8_t*frame,size_t frame_count,uint8_t*out,size_t cap)
{
    if(frame_count>V42_HDLC_MAX_INFO+2||(!frame&&frame_count))return 0;
    uint8_t raw[V42_HDLC_MAX_INFO+4];
    if(frame_count)memcpy(raw,frame,frame_count);
    uint16_t f=v42_hdlc_fcs(raw,frame_count);
    raw[frame_count]=(uint8_t)f;raw[frame_count+1]=(uint8_t)(f>>8);size_t at=0;int ones=0;
    if(flag(out,cap,&at)<0)return 0;
    for(size_t j=0;j<frame_count+2;j++)for(int b=0;b<8;b++){
        int bit=(raw[j]>>b)&1;if(put(out,cap,&at,bit)<0)return 0;
        if(bit){if(++ones==5){if(put(out,cap,&at,0)<0)return 0;ones=0;}}
        else ones=0;
    }
    if(flag(out,cap,&at)<0)return 0;
    return at;
}

int v42_hdlc_decode_raw(const uint8_t*bits,size_t n,uint8_t*frame,size_t cap)
{
    if(!bits||n<16)return-1;
    size_t first=n,last=n;
    for(size_t i=0;i+8<=n;i++){unsigned v=0;for(int b=0;b<8;b++)v|=(bits[i+b]&1u)<<b;
        if(v==0x7e){if(first==n)first=i+8;else{last=i;break;}}}
    if(first==n||last==n||last<=first)return-1;
    uint8_t raw[V42_HDLC_MAX_INFO+4]={0};size_t octets=0;unsigned byte=0,bits_in=0;int ones=0;
    for(size_t i=first;i<last;i++){
        int bit=bits[i]&1;if(bit){if(++ones>6)return-1;}else{if(ones==5){ones=0;continue;}ones=0;}
        byte|=(unsigned)bit<<bits_in;if(++bits_in==8){if(octets>=sizeof raw)return-1;raw[octets++]=(uint8_t)byte;byte=bits_in=0;}
    }
    if(bits_in||octets<3)return-1;
    uint16_t f=0xffff;for(size_t i=0;i<octets;i++)f=update(f,raw[i]);
    if(f!=0xf0b8)return-1;
    size_t frame_count=octets-2;if(frame_count>cap)return-1;
    if(frame_count&&frame)memcpy(frame,raw,frame_count);
    return(int)frame_count;
}

size_t v42_hdlc_encode(uint8_t address,uint8_t control,const uint8_t*info,
                       size_t n,uint8_t*out,size_t cap)
{
    if(n>V42_HDLC_MAX_INFO||(!info&&n))return 0;
    uint8_t raw[V42_HDLC_MAX_INFO+2];raw[0]=address;raw[1]=control;
    if(n)memcpy(raw+2,info,n);
    return v42_hdlc_encode_raw(raw,n+2,out,cap);
}

int v42_hdlc_decode(const uint8_t*bits,size_t n,uint8_t*address,
                    uint8_t*control,uint8_t*info,size_t cap)
{
    uint8_t raw[V42_HDLC_MAX_INFO+2];int z=v42_hdlc_decode_raw(bits,n,raw,sizeof raw);
    if(z<2||(size_t)(z-2)>cap)return-1;
    if(address)*address=raw[0];
    if(control)*control=raw[1];
    if(z>2&&info)memcpy(info,raw+2,(size_t)z-2);
    return z-2;
}
