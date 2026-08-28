#ifndef SIP_SOFTMODEM_SIP_H
#define SIP_SOFTMODEM_SIP_H
#include <stddef.h>
#include <stdint.h>

#define SIP_FIELD 512
struct sip_request {
    char method[16], uri[256], via[SIP_FIELD], from[SIP_FIELD], to[SIP_FIELD];
    char call_id[256], cseq[128], contact[SIP_FIELD];
    const char *body;
};
struct sip_response {
    int status;char reason[64],via[SIP_FIELD],from[SIP_FIELD],to[SIP_FIELD];
    char call_id[256],cseq[128],contact[SIP_FIELD];const char *body;
};
int sip_parse(char *message, struct sip_request *out);
int sip_parse_response(char *message,struct sip_response *out);
int sip_pcma_endpoint(const char *sdp, char *address, size_t address_size, uint16_t *port);
int sip_make_response(char *out, size_t capacity, const struct sip_request *request,
                      int status, const char *reason, const char *tag,
                      const char *contact, const char *user_agent, const char *body);
int sip_make_sdp(char *out, size_t capacity, const char *address, uint16_t port,
                 const char *origin, const char *session_name);
int sip_make_uac_request(char *out,size_t capacity,const char *method,const char *uri,
                         const char *via,const char *from,const char *to,const char *call_id,
                         unsigned cseq,const char *contact,const char *user_agent,const char *body);
#endif
