#include "muscular.h"

void
rb_hello(void) {
    printf("muscular ruby!\n");
}

bool
muscular_enabled(void)
{
    return getenv("RUBY_HARDEN") != NULL && strcmp("YES", getenv("RUBY_HARDEN")) == 0;
}

static bool
in_list(char* name) {
    const muscular_authorized_entry_t *entry = authorized_entries;
    for (; entry->filename != NULL; entry++) {
        if (strcmp(name, entry->filename) == 0) {
            return true;
        }
    }

    return false;
}

bool
muscular_analyze_backtrace(rb_execution_context_t *ec, int entrypoint)
{
    // BACKTRACE_START = 0
    // ALL_BACKTRACE_LINES = -1
    VALUE bt = rb_ec_backtrace_location_ary(ec, 0, -1, FALSE);
    long bt_len = RARRAY_LEN(bt);
    // printf("bt_len = %ld\n", bt_len);
    for(long i = 0; i < bt_len; i++) {
        VALUE test = RARRAY_AREF(bt, i);
        if (NIL_P(test)) {
            // printf("accessing [%ld] = nil\n", i);
            continue;
        }

        if (!rb_frame_info_p(test)) {
            continue;
        }

        // this works but I don't like handing execution back to ruby
        // VALUE path = rb_funcall(test, rb_intern("path"), 0);

        VALUE path = rb_iseq_path(rb_get_iseq_from_frame_info(test));
        char* path_str = RSTRING_PTR(path);
        const struct rb_iseq_struct *frame_info = rb_get_iseq_from_frame_info(test);
        VALUE method = rb_iseq_method_name(frame_info);
        char* method_name = "unavailable";
        if (!NIL_P(method)) {
            method_name = RSTRING_PTR(method);
        }

        unsigned int line = FIX2INT(rb_iseq_first_lineno(frame_info)) + 1;
        char* filename = basename(path_str);
        if (in_list(filename)) {
            // printf("accessing [%ld] = [XXLIST] !!! (%s:%s:%d)\n", i, filename, method_name, line);
            return false;
        }
    }

    return true;
}
