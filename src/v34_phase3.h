#ifndef SOFTMODEM_V34_PHASE3_H
#define SOFTMODEM_V34_PHASE3_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { V34_PHASE3_CALL, V34_PHASE3_ANSWER } v34_phase3_role;
typedef enum {
    V34_P3_SILENCE, V34_P3_S, V34_P3_S_BAR, V34_P3_MD,
    V34_P3_PP, V34_P3_TRN, V34_P3_J
} v34_phase3_signal;

typedef struct {
    v34_phase3_signal signal;
    uint32_t symbols;
    uint32_t milliseconds;
} v34_phase3_event;

#define V34_PHASE3_MAX_EVENTS 10u
typedef struct {
    v34_phase3_event event[V34_PHASE3_MAX_EVENTS];
    size_t count;
} v34_phase3_plan;

typedef struct {
    const v34_phase3_plan *plan;
    size_t index;
    uint32_t elapsed;
    bool complete;
} v34_phase3_cursor;

bool v34_phase3_build_tx_plan(v34_phase3_role role, uint8_t md_length_35ms,
                              v34_phase3_plan *plan);
bool v34_phase3_cursor_init(v34_phase3_cursor *cursor,
                            const v34_phase3_plan *plan);
const v34_phase3_event *v34_phase3_current(const v34_phase3_cursor *cursor);
bool v34_phase3_advance(v34_phase3_cursor *cursor, uint32_t units);

#endif
