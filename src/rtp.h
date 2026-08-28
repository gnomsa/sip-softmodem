#ifndef SIP_SOFTMODEM_RTP_H
#define SIP_SOFTMODEM_RTP_H
#include <stddef.h>
#include <stdint.h>
struct rtp_sender { uint16_t sequence; uint32_t timestamp, ssrc; };
struct rtp_packet { uint8_t payload_type; uint16_t sequence; uint32_t timestamp, ssrc; const uint8_t *payload; size_t payload_len; };
int rtp_parse(const uint8_t *data, size_t length, struct rtp_packet *out);
size_t rtp_build(struct rtp_sender *sender, const uint8_t *payload, size_t length, uint8_t *out, size_t capacity);
#endif
