#ifndef __NEO_CHANGE_DEFS_H
#define __NEO_CHANGE_DEFS_H

/*
 * Temporary fix for projects that link in both streamhtmlparser and clearsilver.
 * Rename all the htmlparser functions that are duplicated in both libraries.
 * This header file should be included in all .c files that include htmlparser.h
 * jsparser.h or statemachine.h, *before* those include files.
 */

#define entityfilter_delete neo_entityfilter_delete
#define entityfilter_new neo_entityfilter_new
#define entityfilter_process neo_entityfilter_process
#define entityfilter_reset neo_entityfilter_reset
#define htmlparser_attr neo_htmlparser_attr
#define htmlparser_attr_type neo_htmlparser_attr_type
#define htmlparser_delete neo_htmlparser_delete
#define htmlparser_in_attr neo_htmlparser_in_attr
#define htmlparser_in_js neo_htmlparser_in_js
#define htmlparser_in_value neo_htmlparser_in_value
#define htmlparser_is_attr_quoted neo_htmlparser_is_attr_quoted
#define htmlparser_is_js_quoted neo_htmlparser_is_js_quoted
#define htmlparser_js_state neo_htmlparser_js_state
#define htmlparser_new neo_htmlparser_new
#define htmlparser_parse neo_htmlparser_parse
#define htmlparser_reset neo_htmlparser_reset
#define htmlparser_reset_mode neo_htmlparser_reset_mode
#define htmlparser_state neo_htmlparser_state
#define htmlparser_tag neo_htmlparser_tag
#define htmlparser_value_index neo_htmlparser_value_index
#define jsparser_delete neo_jsparser_delete
#define jsparser_new neo_jsparser_new
#define jsparser_parse neo_jsparser_parse
#define jsparser_reset neo_jsparser_reset
#define jsparser_state neo_jsparser_state
#define statemachine_definition_delete neo_statemachine_definition_delete
#define statemachine_definition_new neo_statemachine_definition_new
#define statemachine_definition_populate neo_statemachine_definition_populate
#define statemachine_delete neo_statemachine_delete
#define statemachine_enter_state neo_statemachine_enter_state
#define statemachine_exit_state neo_statemachine_exit_state
#define statemachine_in_state neo_statemachine_in_state
#define statemachine_new neo_statemachine_new
#define statemachine_parse neo_statemachine_parse
#define statemachine_set_state neo_statemachine_set_state
#define statemachine_start_record neo_statemachine_start_record
#define statemachine_stop_record neo_statemachine_stop_record

#endif /* __NEO_CHANGE_DEFS_H */
