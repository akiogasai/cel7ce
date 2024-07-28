#if defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>  // Added for better error handling with fprintf and stderr
#include <string.h> // Added for better string handling

#include "cel7ce.h"
#include "fe.h"

// Enhanced error handling for memory allocation
void *ecalloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL) {
        fprintf(stderr, "Couldn't allocate %zu chunks %zu bytes each\n", nmemb, size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Improved function to handle unreachable code
_Noreturn void __unreachable(const char *file, const char *func, int line) {
    fprintf(stderr, "[BUG] Entered unreachable code at %s:%s:%d.\n", file, func, line);
    exit(EXIT_FAILURE);
}

// Cross-platform function to get the username
char *get_username(void) {
#if defined(_WIN32) || defined(__WIN32__)
    static TCHAR buf[4096];
    DWORD size = sizeof(buf) / sizeof(buf[0]);
 
    if (!GetUserName(buf, &size)) {
        return "root";
    }
    return buf;
#else
    char *u = getenv("USER");
    return u ? u : "root";
#endif
}

// Decode 32-bit unsigned integer from bytes
uint32_t decode_u32_from_bytes(uint8_t *bytes) {
    uint32_t accm = 0;
    for (ssize_t b = 3; b >= 0; --b) {
        accm = (accm << 8) | bytes[b];
    }
    return accm;
}

// Load function enhanced for better error handling and cross-platform support
void load(char *user_filename) {
    bool fileisbin = false;
    char filename[4096] = {0};

    if (user_filename == NULL) {
        fileisbin = true;

#if defined(__linux__)
        readlink("/proc/self/exe", filename, sizeof(filename));
#elif defined(_WIN32) || defined(__WIN32__)
        DWORD sz = GetModuleFileName(NULL, filename, sizeof(filename));
        if (sz == 0) {
            DWORD error = GetLastError();
            switch (error) {
                case ERROR_INSUFFICIENT_BUFFER:
                    fprintf(stderr, "Couldn't get path to executable: path too large\n");
                    exit(EXIT_FAILURE);
                default:
                    fprintf(stderr, "Couldn't get path to executable: unknown error (code %lu)\n", error);
                    exit(EXIT_FAILURE);
            }
        }
#else
        fprintf(stderr, "No cartridge or file provided.\n");
        exit(EXIT_FAILURE);
#endif
    } else {
        strncpy(filename, user_filename, sizeof(filename) - 1);
    }

    struct stat st;
    if (stat(filename, &st) == -1) {
        perror("Cannot stat file");
        exit(EXIT_FAILURE);
    }

    char *filebuf = ecalloc(st.st_size, sizeof(char));
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    fread(filebuf, st.st_size, sizeof(char), fp);
    fclose(fp);

    char *start = filebuf;
    if (fileisbin) {
        size_t last0 = 0;
        for (size_t i = 0; i < (size_t)st.st_size; ++i) {
            if (filebuf[i] == '\0') last0 = i;
        }
        start = &filebuf[last0 + 1];
    }

    // Determine language type
    if (fileisbin) {
        if (!strncmp(start, "#janet\n", 7)) {
            lang = LM_Janet;
        } else {
            lang = LM_Fe;
        }
    } else {
        char *dot = strrchr(filename, '.');
        if (dot && !strcmp(dot, ".fe")) {
            lang = LM_Fe;
        } else if (dot && !strcmp(dot, ".janet")) {
            lang = LM_Janet;
        }
    }

    if (lang == LM_Fe) {
        if (setjmp(fe_error_recover) == 1) {
            load_error = true;
            return;
        }

        FILE *fakefp = fmemopen(start, st.st_size - (start - filebuf), "r");
        assert(fakefp != NULL);

        ssize_t gc = fe_savegc(fe_ctx);
        while (true) {
            fe_Object *obj = fe_readfp(fe_ctx, fakefp);

            if (!obj) break;

            fe_eval(fe_ctx, obj);

            fe_restoregc(fe_ctx, gc);
        }
    } else {
        if (janet_dostring(janet_env, start, filename, NULL) != 0) {
            load_error = true;
            return;
        }
    }

    get_string_global("title", config.title, sizeof(config.title));
    config.width = get_number_global("width");
    config.height = get_number_global("height");
    config.scale = get_number_global("scale");
}

// Improved call_func with better memory management and error handling
void call_func(const char *fnname, const char *arg_fmt, ...) {
    size_t argc = strlen(arg_fmt);
    va_list ap;
    va_start(ap, arg_fmt);

    if (lang == LM_Fe && mode.cur == MT_Normal) {
        fe_Object *fnsym = fe_symbol(fe_ctx, fnname);
        if (fe_type(fe_ctx, fe_eval(fe_ctx, fnsym)) == FE_TFUNC) {
            int gc = fe_savegc(fe_ctx);

            fe_Object **objs = calloc(argc + 1, sizeof(fe_Object *));
            objs[0] = fnsym;

            for (size_t i = 0; i < argc; ++i) {
                switch (arg_fmt[i]) {
                    case 's':
                        objs[i + 1] = fe_string(fe_ctx, va_arg(ap, void *));
                        break;
                    case 'n':
                        objs[i + 1] = fe_number(fe_ctx, (float)va_arg(ap, double));
                        break;
                    default:
                        __unreachable(__FILE__, __func__, __LINE__);
                }
            }

            fe_eval(fe_ctx, fe_list(fe_ctx, objs, argc + 1));
            free(objs);
            fe_restoregc(fe_ctx, gc);
        }
    } else {
        JanetSymbol j_sym = janet_csymbol(fnname);
        JanetBinding j_binding = janet_resolve_ext(janet_env, j_sym);
        if (j_binding.type != JANET_BINDING_NONE) {
            if (!janet_checktype(j_binding.value, JANET_FUNCTION)) {
                janet_panicf("Binding '%s' must be a function", fnname);
            }

            Janet *args = calloc(argc, sizeof(Janet));
            for (size_t i = 0; i < argc; ++i) {
                switch (arg_fmt[i]) {
                    case 's': {
                        char *str = va_arg(ap, void *);
                        args[i] = janet_stringv((const uint8_t *)str, strlen(str));
                        break;
                    }
                    case 'n':
                        args[i] = janet_wrap_number(va_arg(ap, double));
                        break;
                    default:
                        __unreachable(__FILE__, __func__, __LINE__);
                }
            }

            Janet res;
            JanetFiber *fiber = janet_current_fiber();
            JanetFunction *j_fn = janet_unwrap_function(j_binding.value);
            JanetSignal sig = janet_pcall(j_fn, argc, args, &res, &fiber);

            if (sig == JANET_SIGNAL_ERROR) {
                janet_stacktrace(fiber, res);
                mode.cur = MT_Error;
            }

            free(args);
        }
    }

    va_end(ap);
}

// Improved function for retrieving global strings
void get_string_global(char *name, char *buf, size_t sz) {
    if (lang == LM_Fe) {
        ssize_t gc = fe_savegc(fe_ctx);
        fe_Object *var = fe_eval(fe_ctx, fe_symbol(fe_ctx, name));
        if (fe_type(fe_ctx, var) == FE_TSTRING) {
            fe_tostring(fe_ctx, var, buf, sz);
        } else {
            fe_errorf("Global '%s' must be a string", name);
        }
        fe_restoregc(fe_ctx, gc);
    } else if (lang == LM_Janet) {
        JanetSymbol j_namesym = janet_symbol((uint8_t *)name, strlen(name));
        JanetBinding j_binding = janet_resolve_ext(janet_env, j_namesym);

        if (j_binding.type == JANET_BINDING_NONE) {
            janet_panicf("Global '%s' not set", name);
        } else if (j_binding.type != JANET_BINDING_DEF && j_binding.type != JANET_BINDING_VAR) {
            janet_panicf("Global '%s' must be a string definition", name);
        } else {
            if (!janet_checktype(j_binding.value, JANET_STRING)) {
                janet_panicf("Global '%s' must be a string", name);
            }

            const char *str = (char *)janet_unwrap_string(j_binding.value);
            strncpy(buf, str, sz);
        }
    }
}

// Improved function for retrieving global numbers
float get_number_global(char *name) {
    if (lang == LM_Fe) {
        ssize_t gc = fe_savegc(fe_ctx);
        fe_Object *var = fe_eval(fe_ctx, fe_symbol(fe_ctx, name));
        if (fe_type(fe_ctx, var) == FE_TNUMBER) {
            float num = fe_tonumber(fe_ctx, var);
            fe_restoregc(fe_ctx, gc);
            return num;
        } else {
            fe_errorf("Global '%s' must be a number", name);
            fe_restoregc(fe_ctx, gc);
            return 0;
        }
    } else if (lang == LM_Janet) {
        JanetSymbol j_namesym = janet_symbol((uint8_t *)name, strlen(name));
        JanetBinding j_binding = janet_resolve_ext(janet_env, j_namesym);

        if (j_binding.type == JANET_BINDING_NONE) {
            janet_panicf("Global '%s' not set", name);
        } else if (j_binding.type != JANET_BINDING_DEF && j_binding.type != JANET_BINDING_VAR) {
            janet_panicf("Global '%s' must be a number definition", name);
        } else {
            if (!janet_checktype(j_binding.value, JANET_NUMBER)) {
                janet_panicf("Global '%s' must be a number", name);
            }

            return janet_unwrap_number(j_binding.value);
        }
    }
    return 0;
}

// Improved error raising function with printf format
void __attribute__((format(printf, 2, 3))) raise_errorf(enum LangMode lang, const char *fmt, ...) {
    static char buf[512];
    memset(buf, 0x0, sizeof(buf));

    va_list ap;
    va_start(ap, fmt);
    ssize_t len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    assert((size_t) len < sizeof(buf));

    if (lang == LM_Fe) {
        fe_error(fe_ctx, buf);
    } else if (lang == LM_Janet) {
        janet_panic(buf);
    }
}

// Enhanced address checking function with better error handling
void check_user_address(enum LangMode lm, size_t addr, size_t sz, _Bool write) {
    if ((write && bank == BK_Rom) || (addr + sz) >= MEMORY_SIZE) {
        const char *action = write ? "writeable" : "readable";

        if (sz == 1) {
            raise_errorf(lm, "Address [%d]0x%04X not %s.", bank, addr, action);
        } else {
            raise_errorf(lm, "Address [%d]0x%04X...%04X not %s.",
                bank, addr, addr + (sz - 1), action);
        }
    }
}
