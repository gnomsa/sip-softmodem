#ifndef SIP_SOFTMODEM_V8_SESSION_H
#define SIP_SOFTMODEM_V8_SESSION_H
#include "v8.h"
#include <stdint.h>
enum v8_role { V8_CALL_DCE, V8_ANSWER_DCE };
enum v8_state { V8_WAIT, V8_ANSAM, V8_TE, V8_CM, V8_JM, V8_CJ, V8_PAUSE, V8_SELECTED, V8_FAILED };
struct v8_session {
    enum v8_role role;enum v8_state state;struct v8_menu local,remote,joint;
    uint64_t now_ms,deadline_ms;unsigned identical;unsigned selected;
};
void v8_session_init(struct v8_session*s,enum v8_role role,const struct v8_menu*local);
void v8_session_advance(struct v8_session*s,unsigned elapsed_ms);
void v8_session_ansam(struct v8_session*s);
void v8_session_menu(struct v8_session*s,const struct v8_menu*menu);
void v8_session_cj(struct v8_session*s);
#endif
