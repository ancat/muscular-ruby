#include "eval_intern.h"
#include "internal.h"
#include "internal/class.h"
#include "internal/error.h"
#include "internal/vm.h"
#include "iseq.h"
#include "ruby/debug.h"
#include "ruby/encoding.h"
#include "vm_core.h"

#include <libgen.h>

#define MUSCULAR_ENTRY_EVAL      1
#define MUSCULAR_ENTRY_EXEC      2
#define MUSCULAR_ENTRY_BACKQUOTE 3

#define MUSCULAR_EXIT \
    rb_raise(rb_eSecurityError, "please do not do this"); \
    UNREACHABLE_RETURN(Qnil);

typedef struct muscular_authorized_entry_struct {
    const char* filename;
    unsigned int lineno;
} muscular_authorized_entry_t;

static const muscular_authorized_entry_t authorized_entries[] = {
    {.filename = "<internal:gem_prelude>", .lineno = 0},
    {.filename = "irb", .lineno = 0},
    {NULL}
};

void rb_hello(void);
bool muscular_enabled(void);
bool muscular_analyze_backtrace(rb_execution_context_t *ec, int entrypoint);
