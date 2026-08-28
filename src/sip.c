#include "sip.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static void copy_field(char *dst,size_t cap,const char *src,size_t n) {
    if (!cap) return;
    if (n>=cap) n=cap-1;
    memcpy(dst,src,n); dst[n]='\0';
}
static void trim_copy(char *dst,size_t cap,const char *src,size_t n) {
    while (n && isspace((unsigned char)*src)) { src++; n--; }
    while (n && isspace((unsigned char)src[n-1])) n--;
    copy_field(dst,cap,src,n);
}

int sip_parse(char *m, struct sip_request *o) {
    if (!m || !o) return -1;
    memset(o,0,sizeof *o);
    char *line_end=strstr(m,"\r\n"); if (!line_end) return -1;
    *line_end='\0';
    if (sscanf(m,"%15s %255s SIP/2.0",o->method,o->uri)!=2) return -1;
    char *line=line_end+2, *body=strstr(line,"\r\n\r\n");
    if (!body) return -1;
    *body='\0'; o->body=body+4;
    while (*line) {
        char *next=strstr(line,"\r\n"); size_t len=next?(size_t)(next-line):strlen(line);
        char *colon=memchr(line,':',len);
        if (colon) {
            size_t name_len=(size_t)(colon-line); const char *value=colon+1; size_t value_len=len-name_len-1;
#define FIELD(name, member) if (name_len==strlen(name) && !strncasecmp(line,name,name_len)) trim_copy(o->member,sizeof o->member,value,value_len)
            FIELD("Via",via); else FIELD("From",from); else FIELD("To",to);
            else FIELD("Call-ID",call_id); else FIELD("CSeq",cseq); else FIELD("Contact",contact);
#undef FIELD
        }
        if (!next) break;
        line=next+2;
    }
    return (!o->via[0] || !o->from[0] || !o->to[0] || !o->call_id[0] || !o->cseq[0]) ? -1 : 0;
}

int sip_pcma_endpoint(const char *sdp,char *addr,size_t cap,uint16_t *port) {
    if (!sdp || !addr || !port) return -1;
    addr[0]='\0'; *port=0; int pcma=0;
    const char *p=sdp;
    while (*p) {
        const char *end=strchr(p,'\n'); size_t n=end?(size_t)(end-p):strlen(p);
        if (n && p[n-1]=='\r') n--;
        if (n>9 && !strncmp(p,"c=IN IP4 ",9)) trim_copy(addr,cap,p+9,n-9);
        if (n>8 && !strncmp(p,"m=audio ",8)) {
            unsigned value=0; char proto[32]={0}, formats[256]={0};
            if (sscanf(p+8,"%u %31s %255[^\r\n]",&value,proto,formats)==3 && value<=65535) {
                *port=(uint16_t)value; char *tok=strtok(formats," ");
                while (tok) { if (!strcmp(tok,"8")) pcma=1; tok=strtok(NULL," "); }
            }
        }
        if (!end) break;
        p=end+1;
    }
    return addr[0] && *port && pcma ? 0 : -1;
}

int sip_make_response(char *out,size_t cap,const struct sip_request *r,int status,
                      const char *reason,const char *tag,const char *contact,
                      const char *user_agent,const char *body) {
    if (!body) body="";
    const int tagged = strstr(r->to,";tag=") != NULL;
    int n=snprintf(out,cap,"SIP/2.0 %d %s\r\nVia: %s\r\nFrom: %s\r\nTo: %s%s%s\r\n"
        "Call-ID: %s\r\nCSeq: %s\r\nContact: <%s>\r\nServer: %s\r\n%s"
        "Content-Length: %zu\r\n\r\n%s",status,reason,r->via,r->from,r->to,
        tagged?"":";tag=",tagged?"":tag,r->call_id,r->cseq,contact,
        user_agent,*body?"Content-Type: application/sdp\r\n":"",strlen(body),body);
    return n<0 || (size_t)n>=cap ? -1 : n;
}

int sip_make_sdp(char *out,size_t cap,const char *address,uint16_t port,
                 const char *origin,const char *session_name) {
    int n=snprintf(out,cap,"v=0\r\no=%s 1 1 IN IP4 %s\r\ns=%s\r\n"
        "c=IN IP4 %s\r\nt=0 0\r\nm=audio %u RTP/AVP 8\r\na=rtpmap:8 PCMA/8000\r\n"
        "a=ptime:20\r\na=sendrecv\r\n",origin,address,session_name,address,port);
    return n<0 || (size_t)n>=cap ? -1 : n;
}
