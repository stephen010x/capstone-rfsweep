#ifndef UTILS_MACROS_H
#define UTILS_MACROS_H

#include <stdbool.h>

#define STRINGIFY(__0) _STRINGIFY(__0)
#define _STRINGIFY(__0) #__0

#define NOP() do {} while(0)
#define VOID_VAL ((void)0)
#define lenof(__0) (sizeof(__0)/sizeof(*(__0)))

#define stringify(__0) _stringify(__0)
#define _stringify(__0) #__0

//#define __FORCE_INLINE__ inline static __attribute__((always_inline))
//#define __INLINE__ __FORCE_INLINE__
//#define __force_inline __FORCE_INLINE__



// https://gcc.gnu.org/onlinedocs/gcc/Attributes.html
// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html
// https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html
// =========================================
// ========= FUNCTION ATTRIBUTES ===========
// force inline
//#define __force_inline  static inline __attribute__((always_inline))
#define __force_inline __attribute__((always_inline)) static inline
//#define __inline   __force_inline
#define __inline__ __force_inline

#define __noreturn __attribute__((noreturn))
// tells compiler that the return value is dependant only on parameters, and 
// not the state of the program. (eg. no side effects)
// can't read static or global variables
#define __const    __attribute__((const))
// first two arguments are passed as registers
#define __fastcall __attribute__((fastcall))
// first argument passed as register, meant for passing self as first argument
// turns out this is for C++ only I guess
//#define __selfcall __attribute__((thiscall))
// for interrupt handlers
#define __interrupt __attribute__((interrupt))
// allows external library functions (dynamic, probably), to be overridden
#define __weak __attribute__((weak))
// similar to const attribute, only reading global variables is allowed
// Allows for optimization where multiple calls to the same function with the same 
// parameter can be optimized as a single call to the function, and the value reused
#define __pure __attribute__((pure))
#define __unsafe __attribute__((no_stack_protector))
// tells gcc that this function is a hot spot, meaning
// it gets run a lot in the executable, and should be aggresively optimized
// it will also place hot functions into their own section to improve cache locality
#define __hot __attribute__((hot))
// tells gcc that the function is unlikely to be executed.
// will optimize for size rather than speed, marked for improved
// branch prediction, and improved code locality / cache locality
#define __cold __attribute__((cold))
// basically the reverse of inline. All of the function calls made within the function with
// this attribute will inline all of the functions it calls
// I think it will only flatten functions in the same translation unit though.
#define __flatten __attribute__((flatten))
// function gets called before main. I think it has to have (void) arguments
#define __construct __attribute__((constructor))
// function gets called after main returns. I think it has to have (void) arguments
#define __destruct __attribute__((destructor))

// =========================================
// ========= VARIABLE ATTRIBUTES ===========
// align to power of two. Can be applied to struct fields
// GCC defines __BIGGEST_ALIGNMENT__ macro, which can be used for this
#define __align(__n) __attribute__((aligned (__n)))
// calls cleanup function that takes one parameter (pointer to the type)
// once the variable or object exits their scope
// ran in reverse order (last defined, first cleanup)
#define __cleanup(__funct) __attribute__((cleanup (__funct)))
// prevent data from being initilized by c runtime startup code
// can reduce program startup times
#define __noinit __attribute__((noinit))
// will pack structs, ignoring alignment
// TODO: may work on bitfields. requires testing
#define __packed __attribute__((packed))
// I think this will store variables in an allocated spot in the binary file
// so that they persist even when the program is killed
// ELF specific
#define __persist __attribute__((persistent))

#ifdef __DYNAMIC_LIB__
    // this will make a function visable when compiling as a shared library when the 
    // -fvisibility=internal or -fvisibility=hidden compiler flags are set
#   define __share __attribute__((visibility("default")))
#else
#   define __share
#endif


// =========================================
// ========= TYPE ATTRIBUTES ===============
// specifies struct/union endianness
// no effect on fields which are struct/union.
// so you have to specify it for all struct fields
#define __bigendian(__type) __attribute__((scalar_storage_order ("big-endian")))
#define __littleendian(__type) __attribute__((scalar_storage_order ("big-endian")))

// =========================================
// ========= GENERAL ATTRIBUTES ============
// emit warnings of depreciation if used
#define __deprecated __attribute__((deprecated))

// =========================================
// ========= MISC ATTRIBUTES ==============
// for switch case statements. It avoids compiler warnings if the fallthrough was
// intentional
#define __fallthrough __attribute__((fallthrough))
// tells the compiler to assume a statement is true for optimization
// for instance, __assume(x == 42) or __assume(++y == 43)
#define __assume(__statement) __attribute__((assume(__statement)))


// =========================================
// ========= PRAGMAS =======================
// specifies how many times a loop should be unrolled
// must be placed immediately before a for, while, or do
// values of 0 or 1 block any unrolling of a loop
#define __unroll(__n) _Pragma("GCC unroll " #__n)



// Check these out:
// #pragma push_macro("macro_name")
// #pragma pop_macro("macro_name")



#define once(__statement) do {      \
        static bool ran = false;    \
        if (!ran) {                 \
            __statement             \
            NOP();                  \
            ran = true;             \
        }                           \
    } while(0);


#define TYPE_NULL(__type) (*(__type*)NULL)
// example usage:
// typeof(TYPE_NULL(structype).member)

#define DEREF_TYPE(__type) typeof(*(__type)NULL)
// TODO: consider using this for verbosity. Maybe a better name is needed though
//#define FUNCT_ENFORCE(__pfunct) DEREF_TYPE(__pfunct)

//#define MEMBER_TYPE(__type, __memberpath) typeof((((__type)*)NULL)->__memberpath)
#define MEMBER_TYPE(__type, __memberpath) typeof(TYPE_NULL(__type).__memberpath)


#define crash() do {((void(*)(void))NULL)();} while(0)
#define segfault() crash()


#endif /* #ifndef UTILS_MACROS_H */
