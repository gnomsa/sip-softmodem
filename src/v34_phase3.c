#include "v34_phase3.h"

#include <stddef.h>

static void add(v34_phase3_plan *p, v34_phase3_signal s,
                uint32_t symbols, uint32_t milliseconds)
{
    p->event[p->count++] = (v34_phase3_event){s, symbols, milliseconds};
}

bool v34_phase3_build_tx_plan(v34_phase3_role role, uint8_t md_length_35ms,
                              v34_phase3_plan *plan)
{
    if (plan == NULL || (role != V34_PHASE3_CALL && role != V34_PHASE3_ANSWER) ||
        md_length_35ms > 127u)
        return false;
    plan->count = 0;
    if (role == V34_PHASE3_ANSWER)
        add(plan, V34_P3_SILENCE, 0, 70);
    add(plan, V34_P3_S, 128, 0);
    add(plan, V34_P3_S_BAR, 16, 0);
    if (md_length_35ms != 0) {
        add(plan, V34_P3_MD, 0, (uint32_t)md_length_35ms * 35u);
        add(plan, V34_P3_S, 128, 0);
        add(plan, V34_P3_S_BAR, 16, 0);
    }
    add(plan, V34_P3_PP, 6u * 48u, 0);
    add(plan, V34_P3_TRN, 512, 0);
    add(plan, V34_P3_J, 0, 0);
    return true;
}
