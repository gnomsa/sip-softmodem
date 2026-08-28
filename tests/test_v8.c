#include "v8.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"V.8 check failed at %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    struct v8_menu sent={.modes=V8_MODE_V21|V8_MODE_V22|V8_MODE_V32|V8_MODE_V34,.lapm=1,.digital_access=1},got;uint8_t bits[128];
    size_t n=v8_encode_menu(&sent,bits,sizeof bits);CHECK(n==80);CHECK(v8_decode_menu(bits,n,&got)==0);CHECK(got.modes==sent.modes);CHECK(got.lapm&&got.digital_access);
    unsigned joint=v8_joint_modes(sent.modes,V8_MODE_V21|V8_MODE_V22);CHECK(joint==(V8_MODE_V21|V8_MODE_V22));CHECK(v8_select_mode(joint)==V8_MODE_V22);
    CHECK(v8_select_mode(V8_MODE_V21|V8_MODE_V32)==V8_MODE_V32);CHECK(v8_encode_cj(bits,sizeof bits)==30);
    for(int frame=0;frame<3;frame++){CHECK(bits[frame*10]==0&&bits[frame*10+9]==1);for(int i=1;i<9;i++)CHECK(bits[frame*10+i]==0);}
    puts("V.8 menu tests passed");return 0;
}
