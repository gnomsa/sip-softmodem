#ifndef SIP_SOFTMODEM_V32BIS_MAP_H
#define SIP_SOFTMODEM_V32BIS_MAP_H
struct v32bis_point {int i,q;};
int v32bis_map_point(int rate,unsigned label,struct v32bis_point*point);
int v32bis_map_nearest(int rate,double i,double q,unsigned*label,double*distance2);
#endif
