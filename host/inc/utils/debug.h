#ifndef HYPERC_DEBUG_H
#define HYPERC_DEBUG_H

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
//#define __FILENAME__ filename_from_path(__FILE__)
#define __FILENAME__ __FILE__


#define ANSI_DEFAULT   "\033[0m"
#define ANSI_BLACK     "\033[0;30m"
#define ANSI_RED       "\033[0;31m"
#define ANSI_YELLOW    "\033[0;33m"
#define ANSI_BLUE      "\033[0;34m"

#define ANSI_BG_RED     "\033[41m"

#define ANS_BLACK_REDBG "\033[1;30;41m"

#define STR_RED(__str)    ANSI_RED    __str ANSI_DEFAULT
#define STR_YELLOW(__str) ANSI_YELLOW __str ANSI_DEFAULT
#define STR_BLUE(__str)   ANSI_BLUE   __str ANSI_DEFAULT

//#define STR_FATAL   ANSI_BLACK ANSI_REDBG "[fatal]"   ANSI_DEFAULT
#define STR_FATAL   ANS_BLACK_REDBG "[fatal]"   ANSI_DEFAULT
#define STR_ERROR   ANSI_RED        "[error]"   ANSI_DEFAULT
#define STR_WARN    ANSI_YELLOW     "[warn] "   ANSI_DEFAULT
#define STR_DEBUG                   "[debug]"
#define STR_MSG                     "[msg]"


#define SYSTEM_MSG(__status, __format, ...) do {                                    \
        printf(__status "\t" STR_BLUE("%s") ":" STR_BLUE("%s") ": " __format "\n",  \
            __FILENAME__,  STRINGIFY(__LINE__) __VA_OPT__(,) __VA_ARGS__);          \
    } while(0)


#define SYSTEM_ERR(__status, __format, ...) do {                                            \
        printf(__status "\t" STR_BLUE(__FILENAME__) ":" STR_BLUE(STRINGIFY(__LINE__)) ": "  \
            STR_BLUE("%s(") "..." STR_BLUE(")") ": " __format "\n",                         \
            __func__ __VA_OPT__(,) __VA_ARGS__);                                            \
    } while(0)


#define IGNORE_UNUSED(__expr)                               \
    _Pragma("GCC diagnostic push")                          \
    _Pragma("GCC diagnostic ignored \"-Wunused-value\"")    \
    __expr                                                  \
    _Pragma("GCC diagnostic pop")



// TODO: Implement later
const char* filename_from_path(const char* path);



#ifndef __DEBUG__


#   define fatal(__ecode) do { IGNORE_UNUSED(                   \
        exit(__ecode);                                          \
    )} while(0)


#   define error(__rcode) do { IGNORE_UNUSED(                   \
        return (__rcode);                                       \
    )} while(0)


#   define fassert(__expr) do { IGNORE_UNUSED(                  \
        if (!(__expr)) abort();                                 \
    )} while(0)

#   undef  assert
#   define assert(__expr, __rcode) do { IGNORE_UNUSED(          \
        if (!(__expr)) return (__rcode);                        \
    )} while(0)

#   define vassert(__expr) do { IGNORE_UNUSED(                  \
        if (!(__expr)) return;                                  \
    )} while(0)

#   define wassert(__expr)  NOP()
//#   define error(__msg)     abort()
#   define warn(__msg)      NOP()
//#   define ffatal(...)      abort()
//#   define ferror(...)      abort()
#   define warnf(...)       NOP()

#   define debugf(...)      NOP()


#   define DEBUG(__statement)
#   define debug(__statement) NOP()


#else


//extern char *assert_format;


#   define fatal(__ecode) do { IGNORE_UNUSED(                                   \
        SYSTEM_ERR(STR_FATAL, "fatal error with code '%d'.", __ecode);          \
        fflush(stdout);                                                         \
        exit(__ecode);                                                          \
    )} while(0)



// "fatal assertion '%s' failed."
#   define fassert(__expr) do { IGNORE_UNUSED(                                  \
        if (!(__expr)) {                                                        \
            SYSTEM_ERR(STR_FATAL, "fatal assertion '%s'.", #__expr);            \
            fflush(stdout);                                                     \
            abort();                                                            \
        }                                                                       \
    )} while(0)

// TODO: use __VA_OPT__ to merge assert and vassert
// TODO: I should probably change these from macros to functions at this point
// "assertion '%s' failed. return code %d."
#   undef  assert
#   define assert(__expr, __rcode) do { IGNORE_UNUSED(                          \
        if (!(__expr)) {                                                        \
            SYSTEM_ERR(STR_ERROR, "assertion '%s'.", #__expr);                  \
            return (__rcode);                                                   \
        }                                                                       \
    )} while(0)

// "assertion '%s' failed."
#   define vassert(__expr) do { IGNORE_UNUSED(                                  \
        if (!(__expr)) {                                                        \
            SYSTEM_ERR(STR_ERROR, "assertion '%s'.", #__expr);                  \
            return;                                                             \
        }                                                                       \
    )} while(0)

// "warning assertion '%s' failed."
#   define wassert(__expr) do { IGNORE_UNUSED(                                  \
        if (!(__expr)) {                                                        \
            SYSTEM_ERR(STR_WARN, "warning assertion '%s'.", #__expr);           \
        }                                                                       \
    )} while(0)

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


#   define DEBUG(__statement) __statement


#   define debug(__statement) do {      \
            __statement                 \
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



#endif /* #ifndef HYPERC_DEBUG_H */
