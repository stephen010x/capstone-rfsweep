#ifndef ASSERT_H
#define ASSERT_H

//#ifdef __DEBUG__
#include <stdlib.h>
#include <stdio.h>
//#endif
// TODO: copy some of the macros from this to remove it as a header dependancy
#include <assert.h>
// https://github.com/lattera/glibc/blob/master/assert/assert.h

// TODO move debug.h and macros.h to their own library like env or something
#include "utils/macros.h"
//#include "utils.h"


// Caution! not static!
#define __FILENAME__ filename_from_path(__FILE__)


#define ANSI_DEFAULT   "\033[0m"
#define ANSI_BLACK     "\033[0;30m"
#define ANSI_RED       "\033[0;31m"
#define ANSI_YELLOW    "\033[0;33m"
#define ANSI_REDBG     "\033[0;41m"
#define ANSI_BLUE      "\033[0;34m"

#define STR_RED(__str)    ANSI_RED    __str ANSI_DEFAULT
#define STR_YELLOW(__str) ANSI_YELLOW __str ANSI_DEFAULT
#define STR_BLUE(__str)   ANSI_BLUE   __str ANSI_DEFAULT

#define STR_FATAL   ANSI_BLACK ANSI_REDBG "[fatal]"   ANSI_DEFAULT
#define STR_ERROR   ANSI_RED              "[error]"   ANSI_DEFAULT
#define STR_WARN    ANSI_YELLOW           "[warn] "   ANSI_DEFAULT
#define STR_DEBUG                         "[debug]"
#define STR_MSG                           "[msg]"


#define SYSTEM_MSG(__status, __format, ...) do {                                    \
        printf(__status "\t" STR_BLUE("%s") ":" STR_BLUE("%s") ": " __format "\n",  \
            __FILENAME__,  STRINGIFY(__LINE__) __VA_OPT__(,) __VA_ARGS__);        \
    } while(0)




const char* filename_from_path(const char* path);



#ifndef __DEBUG__


#   define fatal(__ecode) do {                                  \
        _Pragma("GCC diagnostic push")                          \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\"")    \
        exit((int)(__ecode));                                   \
        _Pragma("GCC diagnostic pop")                           \
    } while(0)


#   define error(__rcode) do {                                  \
        _Pragma("GCC diagnostic push")                          \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\"")    \
        return(__rcode);                                        \
        _Pragma("GCC diagnostic pop")                           \
    } while(0)


#   define fassert(__expr) do {                                 \
        _Pragma("GCC diagnostic push")                          \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\"")    \
        if (!(__expr)) abort();                                 \
        _Pragma("GCC diagnostic pop")                           \
    } while(0)

#   undef  assert
#   define assert(__expr, __rcode) do {                         \
        _Pragma("GCC diagnostic push")                          \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\"")    \
        if (!(__expr)) return (__rcode);                        \
        _Pragma("GCC diagnostic pop")                           \
    } while(0)

#   define vassert(__expr) do {                                 \
        _Pragma("GCC diagnostic push")                          \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\"")    \
        if (!(__expr)) return;                                  \
        _Pragma("GCC diagnostic pop")                           \
    } while(0)

#   define wassert(__expr)  NOP()
//#   define error(__msg)     abort()
#   define warn(__msg)      NOP()
//#   define ffatal(...)      abort()
//#   define ferror(...)      abort()
#   define warnf(...)       NOP()

#   define debugf(...)      NOP()


#   define debug(__statement) NOP()


#else


//extern char *assert_format;


#   define fatal(__ecode) do {
        _Pragma("GCC diagnostic push")                                          \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\"")                    \
        printf(STR_FATAL "\t%s:%s: %s(...): fatal error with code '%d'.\n",     \
            __FILENAME__,  STRINGIFY(__LINE__), __ASSERT_FUNCTION, __ecode);    \
        fflush(stdout);                                                         \
        exit(__ecode);                                                          \
        _Pragma("GCC diagnostic pop")                                           \
    } while(0)



#   define fassert(__expr) do {                                                 \
        _Pragma("GCC diagnostic push")                                          \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\"")                    \
        if (!(__expr)) {                                                        \
            printf(STR_FATAL "\t%s:%s: %s(...): assertion '%s' failed.\n",      \
                __FILENAME__,  STRINGIFY(__LINE__), __ASSERT_FUNCTION, #__expr);\
            fflush(stdout);                                                     \
            abort();                                                            \
        }                                                                       \
        _Pragma("GCC diagnostic pop")                                           \
    } while(0)

// TODO: use __VA_OPT__ to merge assert and vassert
// TODO: I should probably change these from macros to functions at this point
#   undef  assert
#   define assert(__expr, __rcode) do {                                         \
        _Pragma("GCC diagnostic push")                                          \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\"")                    \
        if (!(__expr)) {                                                        \
            printf(STR_ERROR "\t%s:%s: %s(...): "                               \
                "assertion '%s' failed. return code %x.\n",                     \
                __FILENAME__,  STRINGIFY(__LINE__), __ASSERT_FUNCTION,          \
                #__expr, (unsigned int)__rcode);                                \
            return (__rcode);                                                   \
        }                                                                       \
        _Pragma("GCC diagnostic pop")                                           \
    } while(0)

#   define vassert(__expr) do {                                                 \
        _Pragma("GCC diagnostic push")                                          \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\"")                    \
        if (!(__expr)) {                                                        \
            printf(STR_ERROR "\t%s:%s: %s(...): assertion '%s' failed.\n",      \
                __FILENAME__,  STRINGIFY(__LINE__), __ASSERT_FUNCTION, #__expr);\
            return;                                                             \
        }                                                                       \
        _Pragma("GCC diagnostic pop")                                           \
    } while(0)

#   define wassert(__expr) do {                                                 \
        _Pragma("GCC diagnostic push")                                          \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\"")                    \
        if (!(__expr)) {                                                        \
            printf(STR_WARN "\t%s:%s: %s(...): assertion '%s' failed\n",        \
                __FILENAME__,  STRINGIFY(__LINE__), __ASSERT_FUNCTION, #__expr);\
        }                                                                       \
        _Pragma("GCC diagnostic pop")                                           \
    } while(0)

/*#   define fatal(__msg) do {            \
        printf(STR_FATAL "\t" __msg);   \
        abort();                        \
    } while(0)

#   define error(__msg, ...) _error(__msg, __VA_ARGS__, ((void)0))
#   define _error(__msg, __rcode, ...) do {     \
        printf(STR_ERROR "\t" __msg);           \
        return __rcode;                         \
    } while(0)*/

#   define warn(__msg) warnf(__msg)

//#   define ferror(...) do {} while(0)

    // WARNING: Requires static literal for __format
#   define warnf(__format, ...) do {                                \
        SYSTEM_MSG(STR_WARN, __format __VA_OPT__(,) __VA_ARGS__);  \
    } while(0)


#   define debug(__statement) do {      \
        __statement                     \
    } while(0)


/*#   define debugf(__format, ...) do {                               \
        printf(STR_DEBUG "\t%s:%s: " __format "\n", __FILENAME__,   \
            STRINGIFY(__LINE_C__) __VA_OPT__(,) __VA_ARGS__);         \
    } while(0)*/

#   define debugf(__format, ...)                                    \
        SYSTEM_MSG(STR_DEBUG, __format __VA_OPT__(,) __VA_ARGS__);


#endif



#define msgf(__format, ...)                                         \
        SYSTEM_MSG(STR_MSG, __format __VA_OPT__(,) __VA_ARGS__);



#endif
