#include "pcma.h"
#include "v21.h"
#include "v22.h"
#include <stdio.h>

static void pattern(unsigned char*p,size_t n,unsigned seed){unsigned x=seed;for(size_t i=0;i<n;i++){x^=x<<13;x^=x>>17;x^=x<<5;p[i]=(unsigned char)x;}}
static size_t errors(const unsigned char*a,const unsigned char*b,size_t n){size_t e=0;for(size_t i=0;i<n;i++){unsigned x=a[i]^b[i];while(x){e+=x&1u;x>>=1;}}return e;}
static void channel(int16_t*s){uint8_t wire[160];pcma_encode_buffer(s,wire,160);pcma_decode_buffer(wire,s,160);}

static int test_v21(void){
    enum{N=128};unsigned char ab[N],ba[N],rab[N]={0},rba[N]={0};pattern(ab,N,1);pattern(ba,N,2);
    struct v21 a,b;v21_init(&a);v21_init(&b);v21_set_answer_role(&a,0);v21_set_answer_role(&b,1);v21_write(&a,ab,N);v21_write(&b,ba,N);
    size_t na=0,nb=0;int blocks=0;
    for(;blocks<4000&&(na<N||nb<N);blocks++){int16_t xa[160],xb[160];v21_generate(&a,xa,160);v21_generate(&b,xb,160);channel(xa);channel(xb);v21_receive(&a,xb,160);v21_receive(&b,xa,160);na+=v21_read(&b,rab+na,N-na);nb+=v21_read(&a,rba+nb,N-nb);}
    size_t ea=errors(ab,rab,na),eb=errors(ba,rba,nb);double seconds=blocks*0.020;
    printf("V.21/PCMA: A->B %zu/%d bytes, %zu bit errors; B->A %zu/%d bytes, %zu bit errors; %.1f B/s useful\n",na,N,ea,nb,N,eb,N/seconds);
    return na==N&&nb==N&&ea==0&&eb==0?0:1;
}
static int test_v22(void){
    enum{N=256};unsigned char ab[N],ba[N],rab[N]={0},rba[N]={0};pattern(ab,N,3);pattern(ba,N,4);
    struct v22 a,b;v22_init(&a);v22_init(&b);v22_set_answer_role(&a,0);v22_set_answer_role(&b,1);v22_write(&a,ab,N);v22_write(&b,ba,N);
    size_t na=0,nb=0;int blocks=0;
    for(;blocks<4000&&(na<N||nb<N);blocks++){int16_t xa[160],xb[160];v22_generate(&a,xa,160);v22_generate(&b,xb,160);channel(xa);channel(xb);v22_receive(&a,xb,160);v22_receive(&b,xa,160);na+=v22_read(&b,rab+na,N-na);nb+=v22_read(&a,rba+nb,N-nb);}
    size_t ea=errors(ab,rab,na),eb=errors(ba,rba,nb);double seconds=blocks*0.020;
    printf("V.22/PCMA: A->B %zu/%d bytes, %zu bit errors; B->A %zu/%d bytes, %zu bit errors; %.1f B/s including training\n",na,N,ea,nb,N,eb,N/seconds);
    return na==N&&nb==N&&ea==0&&eb==0?0:1;
}
int main(void){int a=test_v21(),b=test_v22();return a||b;}
