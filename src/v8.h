#ifndef SIP_SOFTMODEM_V8_H
#define SIP_SOFTMODEM_V8_H
#include <stddef.h>
#include <stdint.h>

enum v8_mode {
    V8_MODE_V21=1u<<0,
    V8_MODE_V22=1u<<1,
    V8_MODE_V32=1u<<2,
    V8_MODE_V34=1u<<3,
    V8_MODE_V90_ANALOG=1u<<4,
    V8_MODE_V90_DIGITAL=1u<<5
};
struct v8_menu { unsigned modes; int lapm, cellular, digital_access; };

/* Bits are stored one bit per byte, in transmission order. */
size_t v8_encode_menu(const struct v8_menu *menu,uint8_t *bits,size_t capacity);
int v8_decode_menu(const uint8_t *bits,size_t count,struct v8_menu *menu);
unsigned v8_joint_modes(unsigned call_modes,unsigned answer_modes);
unsigned v8_select_mode(unsigned joint_modes);
size_t v8_encode_cj(uint8_t *bits,size_t capacity);
#endif
