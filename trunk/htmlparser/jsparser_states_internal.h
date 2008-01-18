// Generated file. Do not edit.

#define NUM_STATES 10

enum state_internal_enum {
    STATE_I_JS_TEXT,
    STATE_I_JS_Q,
    STATE_I_JS_Q_E,
    STATE_I_JS_DQ,
    STATE_I_JS_DQ_E,
    STATE_I_JS_COMMENT,
    STATE_I_JS_COMMENT_LN,
    STATE_I_JS_COMMENT_ML,
    STATE_I_JS_COMMENT_ML_CLOSE,
    STATE_I_JS_LT,
};

static state states_external[] = {
    JS_STATE_TEXT,
    JS_STATE_Q,
    JS_STATE_Q,
    JS_STATE_DQ,
    JS_STATE_DQ,
    JS_STATE_COMMENT,
    JS_STATE_COMMENT,
    JS_STATE_COMMENT,
    JS_STATE_COMMENT,
    JS_STATE_COMMENT,
};

static const char *states_external_name[] = {
    "text",
    "q",
    "dq",
    "comment",
};

static struct statetable_transitions_s state_transitions[] = {
    { ":default:", STATE_I_JS_COMMENT_ML_CLOSE, STATE_I_JS_COMMENT_ML },
    { "/", STATE_I_JS_COMMENT_ML_CLOSE, STATE_I_JS_TEXT },
    { ":default:", STATE_I_JS_COMMENT_ML, STATE_I_JS_COMMENT_ML },
    { "*", STATE_I_JS_COMMENT_ML, STATE_I_JS_COMMENT_ML_CLOSE },
    { ":default:", STATE_I_JS_COMMENT_LN, STATE_I_JS_COMMENT_LN },
    { "\r\n", STATE_I_JS_COMMENT_LN, STATE_I_JS_TEXT },
    { ":default:", STATE_I_JS_COMMENT, STATE_I_JS_TEXT },
    { "*", STATE_I_JS_COMMENT, STATE_I_JS_COMMENT_ML },
    { "/", STATE_I_JS_COMMENT, STATE_I_JS_COMMENT_LN },
    { ":default:", STATE_I_JS_DQ_E, STATE_I_JS_DQ },
    { ":default:", STATE_I_JS_DQ, STATE_I_JS_DQ },
    { "\"", STATE_I_JS_DQ, STATE_I_JS_TEXT },
    { "\\", STATE_I_JS_DQ, STATE_I_JS_DQ_E },
    { ":default:", STATE_I_JS_Q_E, STATE_I_JS_Q },
    { ":default:", STATE_I_JS_Q, STATE_I_JS_Q },
    { "'", STATE_I_JS_Q, STATE_I_JS_TEXT },
    { "\\", STATE_I_JS_Q, STATE_I_JS_Q_E },
    { ":default:", STATE_I_JS_TEXT, STATE_I_JS_TEXT },
    { "/", STATE_I_JS_TEXT, STATE_I_JS_COMMENT },
    { "\"", STATE_I_JS_TEXT, STATE_I_JS_DQ },
    { "'", STATE_I_JS_TEXT, STATE_I_JS_Q },
    { NULL, STATEMACHINE_ERROR, STATEMACHINE_ERROR }
};
