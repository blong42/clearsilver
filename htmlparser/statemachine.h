// Author: falmeida@google.com (Filipe Almeida)

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

/* TODO(falmeida): I'm not sure about these limits, but since right now we only
 * have 24 states it should be fine */
#define STATEMACHINE_ERROR 127

#ifdef DEBUG
  #define debug printf
#else
  #define debug(x, ...)
#endif

typedef char state;

struct statetable_transitions_s {
  const char *condition;
  state source;
  state destination;
};

struct statemachine_ctx_s;

typedef void(*state_event_function)(struct statemachine_ctx_s *, state, char,
                                    state);

typedef struct statemachine_definition_s {
    int states;
    state **transition_table;
    state_event_function *in_state_events;
    state_event_function *enter_state_events;
    state_event_function *exit_state_events;
} statemachine_definition;

typedef struct statemachine_ctx_s {
    state current_state;
    state next_state;
    statemachine_definition *definition;
    char current_char;
    char *record_buffer;
    int record_pos;
    int record_left;
    void *user;       /* storage space for the layer above */
} statemachine_ctx;

void statetable_populate(state **st, struct statetable_transitions_s *tr);
state **statetable_new(int states);

void statemachine_in_state(statemachine_definition *def, state st,
                           state_event_function func);
void statemachine_enter_state(statemachine_definition *def, state st,
                                     state_event_function func);
void statemachine_exit_state(statemachine_definition *def, state st,
                                    state_event_function func);

statemachine_definition *statemachine_definition_new(int states);
void statemachine_definition_delete(statemachine_definition *def);

void statemachine_start_record(statemachine_ctx *ctx, char *str, int len);
void statemachine_stop_record(statemachine_ctx *ctx);
statemachine_ctx *statemachine_new(statemachine_definition *def);
state statemachine_parse(statemachine_ctx *ctx, const char *str, int size);

void statemachine_delete(statemachine_ctx *ctx);

#endif /* STATEMACHINE_H */
