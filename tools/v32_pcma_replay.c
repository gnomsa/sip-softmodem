#include "pcma.h"
#include "v32_session.h"
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int unsigned_arg(const char *text,unsigned *value)
{
    char *end=0;errno=0;unsigned long parsed=strtoul(text,&end,10);
    if(errno||!text[0]||*end||parsed>UINT32_MAX)return -1;
    *value=(unsigned)parsed;return 0;
}

int main(int argc,char **argv)
{
    if(argc<3||argc>5){
        fprintf(stderr,"usage: %s INPUT.pcma OUTPUT.bin [START_BLOCK [RATE]]\n",
                argv[0]);
        return 2;
    }
    unsigned start=330,rate=9600;
    if((argc>=4&&unsigned_arg(argv[3],&start)<0)||
       (argc>=5&&unsigned_arg(argv[4],&rate)<0)||
       (rate!=7200&&rate!=9600&&rate!=12000&&rate!=14400)){
        fprintf(stderr,"invalid start block or V.32bis rate\n");return 2;
    }
    FILE *input=fopen(argv[1],"rb");
    if(!input){fprintf(stderr,"%s: %s\n",argv[1],strerror(errno));return 1;}
    FILE *output=fopen(argv[2],"wb");
    if(!output){fprintf(stderr,"%s: %s\n",argv[2],strerror(errno));fclose(input);return 1;}
    if(fseek(input,0,SEEK_END)!=0){fprintf(stderr,"seek failed\n");return 1;}
    long length=ftell(input);
    if(length<0||fseek(input,(long)start*160L,SEEK_SET)!=0){
        fprintf(stderr,"input is shorter than requested start block\n");return 1;
    }
    struct v32_session session;
    v32bis_session_init(&session,V32_STD_CALL,(int)rate);
    v32_session_start_standard(&session);
    uint8_t law[160],bytes[512];int16_t receive[160],transmit[160];
    size_t total=0,blocks=0;
    while(fread(law,1,sizeof law,input)==sizeof law){
        pcma_decode_buffer(law,receive,160);
        v32_session_generate(&session,transmit,160);
        v32_session_receive(&session,receive,160);
        size_t count=v32_session_read(&session,bytes,sizeof bytes);
        if(count&&fwrite(bytes,1,count,output)!=count){
            fprintf(stderr,"write failed: %s\n",strerror(errno));return 1;
        }
        total+=count;blocks++;
    }
    if(ferror(input)){fprintf(stderr,"read failed: %s\n",strerror(errno));return 1;}
    if(fclose(output)!=0){fprintf(stderr,"close failed: %s\n",strerror(errno));return 1;}
    fclose(input);
    double evm=session.bis_rx_eq.power>0.0?
        100.0*sqrt(session.bis_rx_eq.error/session.bis_rx_eq.power):0.0;
    double correlations=session.bis_rx_eq.phase_count>1?
        (double)(session.bis_rx_eq.phase_count-1):1.0;
    double carrier_confidence=hypot(
        session.bis_rx_eq.carrier_correlation_i,
        session.bis_rx_eq.carrier_correlation_q)/correlations;
    double carrier_offset=atan2(
        session.bis_rx_eq.carrier_correlation_q,
        session.bis_rx_eq.carrier_correlation_i)*2400.0/(2.0*M_PI);
    fprintf(stdout,
        "start=%u blocks=%zu phase=%s rate=%d bis=%d E=%d connected=%d "
        "mark=%u/%u skip=%u timing=%d previous=%u alignment=%+d acquisition=%d/%d "
        "retrain=%d EVM=%.2f%% carrier=%+.3fHz/%.3f bytes=%zu\n",
        start,blocks,v32_startup_phase_name(session.startup.phase),
        v32_session_rate(&session),session.startup.bis_selected,
        session.remote_e,v32_session_connected(&session),session.tx_marking,
        session.rx_marking,session.bis_rx_skipped,
        session.bis_rx_selected_phase,session.bis_rx_selected_previous,
        session.bis_rx_selected_alignment,
        session.bis_rx_acquisition_complete,session.bis_rx_acquisition_ok,
        session.retrain.state,evm,carrier_offset,carrier_confidence,total);
    return 0;
}
