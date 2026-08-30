#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint16_t fcs_update(uint16_t fcs,uint8_t byte)
{
    fcs^=byte;
    for(unsigned bit=0;bit<8;bit++)
        fcs=(uint16_t)((fcs&1u)?(fcs>>1)^0x8408u:fcs>>1);
    return fcs;
}

int main(int argc,char **argv)
{
    if(argc!=2){fprintf(stderr,"usage: %s DECODED.bin\n",argv[0]);return 2;}
    FILE *input=fopen(argv[1],"rb");
    if(!input){fprintf(stderr,"%s: %s\n",argv[1],strerror(errno));return 1;}
    unsigned long frames=0,valid=0,octets=0;
    uint16_t fcs=0xffffu;size_t length=0;int active=0,escaped=0;
    int value;
    while((value=fgetc(input))!=EOF){
        uint8_t byte=(uint8_t)value;
        if(byte==0x7eu){
            if(active&&length>=4){
                frames++;
                if(fcs==0xf0b8u){valid++;octets+=length;}
            }
            active=1;escaped=0;length=0;fcs=0xffffu;
            continue;
        }
        if(!active)continue;
        if(escaped){byte^=0x20u;escaped=0;}
        else if(byte==0x7du){escaped=1;continue;}
        fcs=fcs_update(fcs,byte);length++;
    }
    if(ferror(input)){fprintf(stderr,"read failed: %s\n",strerror(errno));return 1;}
    fclose(input);
    printf("PPP frames=%lu valid-FCS=%lu valid-octets=%lu\n",frames,valid,octets);
    return valid?0:1;
}
