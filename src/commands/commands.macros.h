////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Name to kinds

#define COMMAND_KIND_DEFS(M) /*
*/ M(OpenFile) /*
*/ M(FocusView) /*
*/ M(GoTo) /*
*/

#define COMMAND_DEFS(M) /*
*/ M("open", OpenFile) /*
*/ M("focus_view", FocusView) /*
*/ M("goto", GoTo) /*
*/

////////////////////////////////////////////////////////////////////////////////

#ifdef MACROS_H
/*  
 *  Options: 
 */

#define DEFINE_COMMAND_KIND_ENUM(name) \
    CMD_Kind_##name,

typedef enum {
    CMD_Kind_NONE,
COMMAND_KIND_DEFS(DEFINE_COMMAND_KIND_ENUM)
    CMD_Kind_COUNT,
} CMD_Kind;

#define DEFINE_COMMAND_ADD(name, kind) \
    cmd_add(S8(name), CMD_Kind_##kind);

#define GENERATE_COMMANDS_ADD() COMMAND_DEFS(DEFINE_COMMAND_ADD)

#endif
