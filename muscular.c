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
in_list(char* name, int line_number) {
    const muscular_authorized_entry_t *entry = authorized_entries;
    for (; entry->filename != NULL; entry++) {
        if (strcmp(name, entry->filename) == 0 && line_number == entry->lineno) {
            return true;
        }
    }

    return false;
}

const char*
entrypoint2str(int entrypoint)
{
    switch (entrypoint) {
        case MUSCULAR_ENTRY_EVAL:
            return "entry_eval";
        case MUSCULAR_ENTRY_EXEC:
            return "entry_exec";
        case MUSCULAR_ENTRY_BACKQUOTE:
            return "entry_backquote";
        default:
            return "unknown";
    }
}

bool
muscular_analyze_backtrace(rb_execution_context_t *ec, int entrypoint)
{
    // BACKTRACE_START = 0
    // ALL_BACKTRACE_LINES = -1
    VALUE bt = rb_ec_backtrace_location_ary(ec, 0, -1, FALSE);
    long bt_len = RARRAY_LEN(bt);

    for(long i = 0; i < bt_len; i++) {
        VALUE test = RARRAY_AREF(bt, i);
        if (NIL_P(test)) {
            continue;
        }

        if (!rb_frame_info_p(test)) {
            continue;
        }

        // this works but I don't like handing execution back to ruby
        // VALUE path = rb_funcall(test, rb_intern("path"), 0);
        const rb_iseq_t* frame = rb_get_iseq_from_frame_info(test);
        VALUE path = rb_iseq_path(frame);
        char* path_str = RSTRING_PTR(path);
        char* filename = basename(path_str);

        /// NUM2LONG(rb_funcall(location, rb_intern("lineno"), 0)));
        const struct rb_iseq_constant_body *const body = ISEQ_BODY(frame);
        const rb_code_location_t *loc = &body->location.code_location;

        const struct rb_iseq_struct *frame_info = rb_get_iseq_from_frame_info(test);
        VALUE method = rb_iseq_method_name(frame_info);
        const char* method_name = "nil";
        if (!NIL_P(method)) {
            method_name = RSTRING_PTR(method);
        }

        // why is this wrong
        // unsigned int line = FIX2INT(rb_iseq_first_lineno(frame));

        /*
        if (i == 0) {
            printf("---\n");
        }

        printf(
            "(%s) entrypoint=%s; filename=%s(%s); line=%d or line=%d or line=%d\n",
            method_name,
            entrypoint2str(entrypoint),
            path_str,
            filename,
            line,
            other_line,
            rb_sourceline()
        );
        */

        // special exemption, long story and also idk
        if (strcmp("<internal:gem_prelude>", filename) == 0) {
            return false;
        }

        if (in_list(filename, rb_sourceline())) {
            return false;
        }
    }

    return true;
}

void muscular_init(void) {
    if (muscular_enabled()) {
        FILE *handle = fopen("callsites.txt", "r");
        if (handle == NULL) {
            printf("Couldn't load callsite index file!\n");
            exit(1);
        }

        authorized_entries = muscular_load_callsites(handle);
        fclose(handle);
    }
}

muscular_authorized_entry_t * muscular_load_callsites(FILE *handle) {
    int count = 0;

    for (char c = getc(handle); c != EOF; c = getc(handle)) {
        if (c == '\n') {
            count = count + 1;
        }
    }

    if (count == 0) {
        return NULL;
    }

    rewind(handle);

    muscular_authorized_entry_t *entries = calloc(
        count,
        sizeof(muscular_authorized_entry_t) + 1
    );

    entries[count] = (muscular_authorized_entry_t) { NULL };

    char *line = NULL;
    size_t line_length = 0;
    while (getline(&line, &line_length, handle) != -1) {
        size_t path_length = strchr(line, ':') - line;
        char *path = malloc(path_length + 1);
        int line_number;

        strncpy(path, line, path_length);
        path[path_length] = '\0';
        sscanf(line + path_length, ":%d\n", &line_number);
        entries[--count] = (muscular_authorized_entry_t) {
            .filename = path,
            .lineno = line_number
        };
    }

    return entries;
}
