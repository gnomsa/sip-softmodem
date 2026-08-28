#include "v34_framing.h"
#include <stddef.h>
bool v34_frame_geometry_init(v34_frame_geometry*g,v34_symbol_rate s,unsigned rate){static const uint8_t j[V34_SYMBOL_COUNT]={7,8,7,7,7,8};static const uint8_t p[V34_SYMBOL_COUNT]={12,12,14,15,16,15};static const unsigned max[V34_SYMBOL_COUNT]={21600,26400,26400,28800,31200,33600};unsigned n,b,r,c=0,i;uint16_t swp=0;if(!g||(unsigned)s>=V34_SYMBOL_COUNT||rate<2400||rate>max[s]||rate%2400)return false;n=rate*28u/(100u*j[s]);if(n*j[s]*100u!=rate*28u)return false;b=(n+p[s]-1u)/p[s];r=n-(b-1u)*p[s];for(i=0;i<p[s];i++){c+=r;if(c>=p[s]){swp|=(uint16_t)(1u<<(p[s]-1u-i));c-=p[s];}}g->data_frames_per_superframe=j[s];g->mapping_frames_per_data_frame=p[s];g->bits_per_data_frame=n;g->high_frame_bits=b;g->high_frame_count=r;g->switching_pattern=swp;return true;}
bool v34_mapping_frame_high(const v34_frame_geometry*g,unsigned i){if(!g||i>=g->mapping_frames_per_data_frame)return false;return (g->switching_pattern&(1u<<(g->mapping_frames_per_data_frame-1u-i)))!=0;}

bool v34_sync_inversion(const v34_frame_geometry *g,
                        unsigned data_frame,
                        unsigned interval_4d)
{
    /* Table 12, left-most bit first. */
    static const uint16_t patterns[2] = {
        0x1dfeu, /* J = 7: 01 11 01 11 11 11 10 */
        0x77fau  /* J = 8: 01 11 01 11 11 11 10 10 */
    };
    unsigned half;
    unsigned bit;
    uint16_t pattern;

    if (!g || data_frame >= g->data_frames_per_superframe)
        return false;
    if (interval_4d == 0)
        half = 0;
    else if (interval_4d == 2u * g->mapping_frames_per_data_frame)
        half = 1;
    else
        return false;

    pattern = patterns[g->data_frames_per_superframe == 8u];
    bit = 2u * data_frame + half;
    return (pattern & (1u << (2u * g->data_frames_per_superframe - 1u - bit))) != 0;
}
