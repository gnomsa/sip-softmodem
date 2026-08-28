#include "at.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static size_t append(char*out,size_t cap,size_t used,const char*s){size_t n=strlen(s);if(used<cap){size_t room=cap-used;size_t copy=n<room?n:room;memcpy(out+used,s,copy);}return used+n;}
static size_t result(struct at_modem*a,char*out,size_t cap,const char*word,const char*number){if(a->quiet)return 0;char b[96];snprintf(b,sizeof b,"\r\n%s\r\n",a->verbose?word:number);size_t n=strlen(b);if(cap){size_t z=n<cap?n:cap;memcpy(out,b,z);}return n;}
void at_init(struct at_modem*a){memset(a,0,sizeof *a);a->echo=1;a->verbose=1;a->min_speed=300;a->max_speed=1200;}
static int speed_name(const char*s){if(!strncasecmp(s,"V21",3))return 300;if(!strncasecmp(s,"V22B",4))return 2400;if(!strncasecmp(s,"V22",3))return 1200;if(!strncasecmp(s,"V32",3))return 9600;return atoi(s);}
static enum at_event execute(struct at_modem*a,char*out,size_t cap,size_t*used){char*p=a->line;if(strncasecmp(p,"AT",2)){*used+=result(a,out+(*used<cap?*used:cap),*used<cap?cap-*used:0,"ERROR","4");return AT_EVENT_NONE;}p+=2;enum at_event event=AT_EVENT_NONE;int ok=1;
    while(*p&&ok){if(*p==' '){p++;continue;}if(!strncasecmp(p,"D",1)){p++;a->pulse_dial=0;if(*p=='T'||*p=='t'){p++;}else if(*p=='P'||*p=='p'){a->pulse_dial=1;p++;}if(*p=='L'||*p=='l'){snprintf(a->dial_number,sizeof a->dial_number,"%s",a->last_number);p++;}else{size_t z=0;while(*p&&z<sizeof a->dial_number-1){if(isdigit((unsigned char)*p)||strchr("*#+",*p))a->dial_number[z++]=*p;p++;}a->dial_number[z]=0;if(z)snprintf(a->last_number,sizeof a->last_number,"%s",a->dial_number);}if(!a->dial_number[0])ok=0;else event=AT_EVENT_DIAL;}else if(!strncasecmp(p,"A",1)){event=AT_EVENT_ANSWER;p++;}else if(!strncasecmp(p,"H",1)){event=AT_EVENT_HANGUP;p++;if(isdigit((unsigned char)*p))p++;}else if(!strncasecmp(p,"O",1)){event=AT_EVENT_ONLINE;p++;if(isdigit((unsigned char)*p))p++;}else if(!strncasecmp(p,"E",1)){p++;if(*p=='0'||*p=='1')a->echo=*p++-'0';else ok=0;}else if(!strncasecmp(p,"V",1)){p++;if(*p=='0'||*p=='1')a->verbose=*p++-'0';else ok=0;}else if(!strncasecmp(p,"Q",1)){p++;if(*p=='0'||*p=='1')a->quiet=*p++-'0';else ok=0;}else if(!strncasecmp(p,"Z",1)){int s0=a->s0;at_init(a);a->s0=s0;p++;}else if(!strncasecmp(p,"I",1)){*used=append(out,cap,*used,"\r\nSIP Softmodem 0.1 by Gnomsa\r\n");p++;if(isdigit((unsigned char)*p))p++;}else if(!strncasecmp(p,"S0?",3)){char b[32];snprintf(b,sizeof b,"\r\n%d\r\n",a->s0);*used=append(out,cap,*used,b);p+=3;}else if(!strncasecmp(p,"S0=",3)){char*end;long v=strtol(p+3,&end,10);if(end==p+3||v<0||v>255)ok=0;else{a->s0=(int)v;p=end;}}else if(!strncasecmp(p,"+MS?",4)){char b[80];snprintf(b,sizeof b,"\r\nAUTO,0,%d,%d\r\n",a->min_speed,a->max_speed);*used=append(out,cap,*used,b);p+=4;}else if(!strncasecmp(p,"+MS=",4)){p+=4;char*comma=strchr(p,',');char name[16];size_t n=comma?(size_t)(comma-p):strlen(p);if(n>=sizeof name)n=sizeof name-1;memcpy(name,p,n);name[n]=0;a->preferred_speed=!strcasecmp(name,"AUTO")?0:speed_name(name);if(a->preferred_speed!=0&&(a->preferred_speed<a->min_speed||a->preferred_speed>a->max_speed))ok=0;p+=strlen(p);}else ok=0;}
    if(event==AT_EVENT_NONE)
        *used+=result(a,out+(*used<cap?*used:cap),*used<cap?cap-*used:0,
                      ok?"OK":"ERROR",ok?"0":"4");
    return ok?event:AT_EVENT_NONE;
}
enum at_event at_feed(struct at_modem*a,const uint8_t*d,size_t n,char*out,size_t cap){size_t used=0;enum at_event event=AT_EVENT_NONE;for(size_t i=0;i<n;i++){uint8_t c=d[i];if(a->echo&&used<cap)out[used++]=(char)c;if(c=='\r'||c=='\n'){if(a->length){a->line[a->length]=0;event=execute(a,out,cap,&used);a->length=0;}}else if((c==8||c==127)&&a->length)a->length--;else if(c>=32&&c<127&&a->length<sizeof a->line-1)a->line[a->length++]=(char)c;}return event;}
enum at_event at_ring(struct at_modem*a,char*out,size_t cap){a->rings++;size_t n=result(a,out,cap,"RING","2");(void)n;return a->s0>0&&a->rings>=a->s0?AT_EVENT_ANSWER:AT_EVENT_NONE;}
enum at_event at_ring_caller(struct at_modem*a,const char*caller,char*out,size_t cap){a->rings++;size_t n=result(a,out,cap,"RING","2");if(caller&&*caller&&!a->quiet){char b[640];snprintf(b,sizeof b,"\r\n+CLIP: \"%s\"\r\n",caller);size_t used=n,room=used<cap?cap-used:0;if(room){size_t z=strlen(b)<room?strlen(b):room;memcpy(out+used,b,z);}n+=strlen(b);}return a->s0>0&&a->rings>=a->s0?AT_EVENT_ANSWER:AT_EVENT_NONE;}
size_t at_connected(struct at_modem*a,int speed,char*out,size_t cap){a->online=1;a->rings=0;char word[64];snprintf(word,sizeof word,"CONNECT %d",speed);return result(a,out,cap,word,"1");}
size_t at_no_carrier(struct at_modem*a,char*out,size_t cap){a->online=0;a->rings=0;return result(a,out,cap,"NO CARRIER","3");}
size_t at_no_dialtone(struct at_modem*a,char*out,size_t cap){return result(a,out,cap,"NO DIALTONE","6");}
size_t at_busy(struct at_modem*a,char*out,size_t cap){return result(a,out,cap,"BUSY","7");}
size_t at_no_answer(struct at_modem*a,char*out,size_t cap){return result(a,out,cap,"NO ANSWER","8");}
