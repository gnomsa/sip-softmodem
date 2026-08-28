#include "v42_arq.h"
#include <string.h>

#define RX_MASK 8191u
static unsigned distance(unsigned from,unsigned to){return(to-from)&127u;}
void v42_arq_init(struct v42_arq*a,unsigned cr){memset(a,0,sizeof*a);a->cr=cr&1;}
size_t v42_arq_unacked(const struct v42_arq*a){return distance(a->va,a->vs);}

size_t v42_arq_write(struct v42_arq*a,const uint8_t*d,size_t n)
{
    size_t used=0;
    while(used<n&&v42_arq_unacked(a)<V42_ARQ_WINDOW){
        size_t z=n-used;if(z>V42_ARQ_CHUNK)z=V42_ARQ_CHUNK;
        struct v42_arq_slot*s=&a->tx[a->vs];memcpy(s->data,d+used,z);s->count=z;s->valid=1;
        a->vs=(a->vs+1)&127u;used+=z;
    }
    return used;
}

size_t v42_arq_read(struct v42_arq*a,uint8_t*d,size_t n)
{size_t z=0;while(z<n&&a->rh!=a->rt){d[z++]=a->rx[a->rh];a->rh=(a->rh+1)&RX_MASK;}return z;}

static void acknowledge(struct v42_arq*a,unsigned nr)
{
    unsigned outstanding=distance(a->va,a->vs),acked=distance(a->va,nr);
    if(acked>outstanding)return;
    while(a->va!=nr){a->tx[a->va].valid=0;a->va=(a->va+1)&127u;}
    if(distance(a->va,a->send_cursor)>distance(a->va,a->vs))a->send_cursor=a->va;
}

size_t v42_arq_next(struct v42_arq*a,uint8_t*bits,size_t cap)
{
    if(a->have_pending_s){enum v42_lapm_type t=a->pending_s;a->have_pending_s=0;
        return v42_lapm_encode_s(t,a->cr,a->vr,0,bits,cap);}
    if(a->send_cursor==a->vs)return 0;
    struct v42_arq_slot*s=&a->tx[a->send_cursor];if(!s->valid)return 0;
    size_t n=v42_lapm_encode_i(a->cr,a->send_cursor,a->vr,0,s->data,s->count,bits,cap);
    if(n)a->send_cursor=(a->send_cursor+1)&127u;
    return n;
}

int v42_arq_receive(struct v42_arq*a,const uint8_t*bits,size_t n)
{
    struct v42_lapm_frame f;if(v42_lapm_decode(bits,n,&f)<0)return-1;
    if(f.type==V42_LAPM_I){
        acknowledge(a,f.nr);
        if(f.ns!=a->vr){a->pending_s=V42_LAPM_REJ;a->have_pending_s=1;return 0;}
        size_t free=(a->rh-a->rt-1)&RX_MASK;if(f.info_count>free){a->pending_s=V42_LAPM_RNR;a->have_pending_s=1;return 0;}
        for(size_t i=0;i<f.info_count;i++){a->rx[a->rt]=f.info[i];a->rt=(a->rt+1)&RX_MASK;}
        a->vr=(a->vr+1)&127u;a->pending_s=V42_LAPM_RR;a->have_pending_s=1;return 1;
    }
    if(f.type==V42_LAPM_RR||f.type==V42_LAPM_RNR){acknowledge(a,f.nr);return 0;}
    if(f.type==V42_LAPM_REJ){acknowledge(a,f.nr);a->send_cursor=f.nr;return 0;}
    if(f.type==V42_LAPM_SREJ){if(a->tx[f.nr].valid)a->send_cursor=f.nr;return 0;}
    return 0;
}
