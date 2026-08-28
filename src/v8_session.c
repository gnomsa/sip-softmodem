#include "v8_session.h"
#include <string.h>
static int same(const struct v8_menu*a,const struct v8_menu*b){return a->modes==b->modes&&a->lapm==b->lapm&&a->cellular==b->cellular&&a->digital_access==b->digital_access;}
void v8_session_init(struct v8_session*s,enum v8_role role,const struct v8_menu*local){memset(s,0,sizeof *s);s->role=role;s->local=*local;s->state=role==V8_CALL_DCE?V8_WAIT:V8_ANSAM;if(role==V8_ANSWER_DCE)s->deadline_ms=5000;}
void v8_session_ansam(struct v8_session*s){if(s->role==V8_CALL_DCE&&s->state==V8_WAIT){s->state=V8_TE;s->deadline_ms=s->now_ms+1000;}}
void v8_session_menu(struct v8_session*s,const struct v8_menu*m){
    int expected=(s->role==V8_ANSWER_DCE&&s->state==V8_ANSAM)||(s->role==V8_CALL_DCE&&s->state==V8_CM);if(!expected)return;
    if(s->identical&&same(&s->remote,m))s->identical++;else{s->remote=*m;s->identical=1;}
    if(s->identical<2)return;
    s->joint=*m;s->joint.modes=v8_joint_modes(s->local.modes,m->modes);s->joint.lapm=s->local.lapm&&m->lapm;s->selected=v8_select_mode(s->joint.modes);
    if(!s->selected){s->state=V8_FAILED;return;}s->state=s->role==V8_ANSWER_DCE?V8_JM:V8_CJ;
}
void v8_session_cj(struct v8_session*s){if((s->role==V8_ANSWER_DCE&&s->state==V8_JM)||(s->role==V8_CALL_DCE&&s->state==V8_CJ)){s->state=V8_PAUSE;s->deadline_ms=s->now_ms+75;}}
void v8_session_advance(struct v8_session*s,unsigned ms){s->now_ms+=ms;if(!s->deadline_ms||s->now_ms<s->deadline_ms)return;s->deadline_ms=0;if(s->state==V8_TE)s->state=V8_CM;else if(s->state==V8_PAUSE)s->state=V8_SELECTED;else if(s->state==V8_ANSAM)s->state=V8_FAILED;}
