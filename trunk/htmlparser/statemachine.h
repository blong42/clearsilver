/* Copyright 2007 Google Inc. All Rights Reserved.
 * Author: falmeida@google.com (Filipe Almeida)
 */

#ifndef __NEO_STATEMACHINE_H_
#define __NEO_STATEMACHINE_H_

#ifdef __cplusplus
namespace security_streamhtmlparser {
#endif

/* TODO(falmeida): I'm not sure about these limits, but since right now we only
 * have 24 states it should be fine */

enum {
    STATEMACHINE_ERROR = 127
};

struct statetable_transitions_s {
  const char *condition;
  int source;
  int destination;
};

struct statemachine_ctx_s;

typedef void(*state_event_function)(struct statemachine_ctx_s *, int, char,
                                    int);

typedef struct statemachine_definition_s {
    int num_states;
    int **transition_table;
    state_event_function *in_state_events;
    state_event_function *enter_state_events;
    state_event_function *exit_state_events;
} statemachine_definition;

typedef struct statemachine_ctx_s {
    int current_state;
    int next_state;
    statemachine_definition *definition;
    char current_char;
    char *record_buffer;
    int record_pos;
    int record_left;
    void *user;       /* storage space for the layer above */
} statemachine_ctx;

void statemachine_definition_populate(statemachine_definition *def,
                                     const struct statetable_transitions_s *tr);

void statemachine_in_state(statemachine_definition *def, int st,
                           state_event_function func);
void statemachine_enter_state(statemachine_definition *def, int st,
                                     state_event_function func);
void statemachine_exit_state(statemachine_definition *def, int st,
                                    state_event_function func);

statemachine_definition *statemachine_definition_new(int states);
void statemachine_definition_delete(statemachine_definition *def);

int statemachine_get_state(statemachine_ctx *ctx);
void statemachine_set_state(statemachine_ctx *ctx, int state);

void statemachine_start_record(statemachine_ctx *ctx, char *str, int len);
void statemachine_stop_record(statemachine_ctx *ctx);
statemachine_ctx *statemachine_new(statemachine_definition *def);
int statemachine_parse(statemachine_ctx *ctx, const char *str, int size);

void statemachine_delete(statemachine_ctx *ctx);

#ifdef __cplusplus
}  /* namespace security_streamhtmlparser */
#endif

#endif /* __NEO_STATEMACHINE_H_ */
