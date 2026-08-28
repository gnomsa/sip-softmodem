#ifndef SIP_SOFTMODEM_AT_H
#define SIP_SOFTMODEM_AT_H
#include <stddef.h>
#include <stdint.h>
enum at_event { AT_EVENT_NONE, AT_EVENT_ANSWER, AT_EVENT_HANGUP, AT_EVENT_ONLINE, AT_EVENT_DIAL };
struct at_modem {
    char line[256];size_t length;int echo,verbose,quiet,s0,rings,online;
    int min_speed,max_speed,preferred_speed;
    char dial_number[128],last_number[128];int pulse_dial;
};
void at_init(struct at_modem *at);
enum at_event at_feed(struct at_modem *at,const uint8_t *data,size_t length,char *output,size_t capacity);
enum at_event at_ring(struct at_modem *at,char *output,size_t capacity);
size_t at_connected(struct at_modem *at,int speed,char *output,size_t capacity);
size_t at_no_carrier(struct at_modem *at,char *output,size_t capacity);
size_t at_no_dialtone(struct at_modem *at,char *output,size_t capacity);
size_t at_busy(struct at_modem *at,char *output,size_t capacity);
size_t at_no_answer(struct at_modem *at,char *output,size_t capacity);
#endif
