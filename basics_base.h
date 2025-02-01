#if !defined(INCLUDED_BASICS_BASE_H)
#define INCLUDED_BASICS_BASE_H

#if defined __clang__
#define COMPILER_IS_CLANG
#elif defined __GNUC__
#define COMPILER_IS_GNUC
#elif defined _MSC_VER
#define COMPILER_IS_MSC
#else
#error "unsupported compiler"
#endif

#if defined(__unix__)
#define SYSTEM_IS_UNIX
#elif defined(_WIN64) /* NOTE(Emhyr): Win64 because we only support 64-bit systems */
#define SYSTEM_IS_WIN64
#else
#error "unsupported system"
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define ARCHITECTURE_IS_X64
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCHITECTURE_IS_ARM64
#else
#error "unsupported architecture"
#endif

#if defined(__cplusplus) && __cplusplus >= 201103L
#define LANGUAGE_IS_CPP
#elif __STDC_VERSION__ >= 201112L
#define LANGUAGE_IS_C
#else
#error "unsupported language"
#endif

#if defined(ARCHITECTURE_IS_X64)
#include <immintrin.h>
#if defined(SYSTEM_IS_WIN64)
#include <intrin.h>
#endif
#endif

#if defined(LANGUAGE_IS_CPP)
#define EXTERNAL     extern "C"
#define ALIGNAS(...) alignas(__VA_ARGS__)
#define ALIGNOF(...) alignof(__VA_ARGS__)
#define ASSERT(...)  static_assert(__VA_ARGS__)
#else
#define EXTERNAL     extern
#define ALIGNAS(...) _Alignas(__VA_ARGS__)
#define ALIGNOF(...) _Alignof(__VA_ARGS__)
#define ASSERT(...)  _Static_assert(__VA_ARGS__)
#endif

#if defined(COMPILER_IS_CLANG) || defined(COMPILER_IS_GNUC) || defined(COMPILER_IS_MSC)
#define RESTRICT __restrict
#endif

#if defined(COMPILER_IS_CLANG) || defined(COMPILER_IS_GNUC)
#define CCALL       __attribute__((cdecl))
#define STDCALL     __attribute__((stdcall))
#define FASTCALL    __attribute__((fastcall))
#define PUBLIC      __attribute__((visibility("default"))) EXTERNAL
#define IMPORTED    EXTERNAL
#define UNRETURNING __attribute__((noreturn)) void
#define INLINED     __attribute__((always_inline))
#define THREADIC    __thread
#elif defined(COMPILER_IS_MSC)
#define CCALL       __cdecl
#define STDCALL     __stdcall
#define FASTCALL    __fastcall
#define PUBLIC      __declspec(dllexport) EXTERNAL
#define IMPORTED    __declspec(dllimport) EXTERNAL
#define UNRETURNING __declspec(noreturn) void
#define INLINED     __forceinline
#define THREADIC    __declspec(thread)
#endif

#if defined(SYSTEM_IS_WIN64)
#if defined(COMPILER_IS_CLANG) || defined(COMPILER_IS_GNUC)
#define IMMUTABLE __attribute__((section(".rdata"))) const
#elif defined(COMPILER_IS_MSC)
#define IMMUTABLE __declspec(allocate(".rdata")) const
#endif
#elif defined(SYSTEM_IS_UNIX)
#if defined(COMPILER_IS_CLANG) || defined(COMPILER_IS_GNUC)
#define IMMUTABLE __attribute__((section(".rodata"))) const
#endif
#endif

#define MEMBEROF(t, m) (((t *)0)->m)
#define OFFSETOF(t, m) ((long long)&MEMBEROF(t))
#define COUNTOF(a)     (sizeof(a) / sizeof(*(a)))
#define WIDTHOF(x)     (sizeof(x) * 8)

#define STRINGIFY_(s)      #s
#define STRINGIFY(s)       STRINGIFY_(s)
#define CONCATENATE_(a, b) a##b
#define CONCATENATE(a, b)  CONCATENATE_(a, b)

/* NOTE(Emhyr): to hide a global symbol from other translation units */
#define PRIVATE static

/* NOTE(Emhyr): to preserve the storage of a local symbol */
#define PERSISTANT static

#if defined(__clang__) || defined(__GNUC__)
#define FallThrough() __attribute__((fallthrough))
#define DebugTrap()   __builtin_debugtrap()

typedef __builtin_va_list VArgs;
#define BeginVArgs(...) __builtin_va_start(__VA_ARGS__)
#define EndVArgs(...)   __builtin_va_end(__VA_ARGS__)
#define CopyVArgs(...)  __builtin_va_copy(__VA_ARGS__)
#define GetVArg(...)    __builtin_va_arg(__VA_ARGS__)
#elif defined(_MSC_VER)
#define FallThrough() ((void)0)
#define DebugTrap()   __debugbreak()

#include <stdarg.h>
typedef va_list VArgs;
#define BeginVArgs(...) va_start(__VA_ARGS__)
#define EndVArgs(...)   va_end(__VA_ARGS__)
#define CopyVArgs(...)  va_copy(__VA_ARGS__)
#define GetVArg(...)    va_arg(__VA_ARGS__)
#endif

#include <setjmp.h>
typedef jmp_buf JmpContext;
#define SetJmp(...) setjmp(__VA_ARGS__)
#define Jmp(...)    longjmp(__VA_ARGS__)

#define Assert(x, ...) do { if (!(x)) DebugTrap(); } while (0)

typedef unsigned char          U8;
typedef unsigned short int     U16;
typedef unsigned int           U32;
typedef unsigned long long int U64;

#define MAXIMUM_U8  0xff
#define MAXIMUM_U16 0xffff
#define MAXIMUM_U32 0xffffffff
#define MAXIMUM_U64 0xffffffffffffffffllu

typedef signed char          S8;
typedef signed short int     S16;
typedef signed int           S32;
typedef signed long long int S64;

#define MINIMUM_S8  ((S8 )0x80)
#define MINIMUM_S16 ((S16)0x8000)
#define MINIMUM_S32 ((S32)0x80000000)
#define MINIMUM_S64 ((S64)0x8000000000000000llu)
#define MAXIMUM_S8  ((S8 )0x7f)
#define MAXIMUM_S16 ((S16)0x7fff)
#define MAXIMUM_S32 ((S32)0x7fffffff)
#define MAXIMUM_S64 ((S64)0x7fffffffffffffffllu)

typedef float  F32;
typedef double F64;

#define F32_DECIMAL            9
#define F32_PRECISION          6
#define F32_EPSILON            1.192092896e-07f
#define F32_MANTISSA           24
#define F32_MAXIMUM            3.402823466e+38f
#define F32_MAXIMUM_EXPONENT10 38
#define F32_MAXIMUM_EXPONENT2  128
#define F32_MINIMUM            1.401298464e-45f
#define F32_MININUM_NORMALIZED 1.175494351e-38f
#define F32_MININUM_EXPONENT10 (-37)
#define F32_MINIMUM_EXPONENT2  (-125)

#define F64_DECIMAL            17
#define F64_PRECISION          15
#define F64_EPSILON            2.2204460492503131e-016
#define F64_MANTISSA           53
#define F64_MAXIMUM            1.7976931348623158e+308
#define F64_MAXIMUM_EXPONENT10 308
#define F64_MAXIMUM_EXPONENT2  1024
#define F64_MINIMUM            4.9406564584124654e-324
#define F64_MININUM_NORMALIZED 2.2250738585072014e-308
#define F64_MININUM_EXPONENT10 (-307)
#define F64_MINIMUM_EXPONENT2  (-1021)

typedef unsigned char          Bits8;
typedef unsigned short int     Bits16;
typedef unsigned int           Bits32;
typedef unsigned long long int Bits64;

typedef char      Byte;
typedef long long Word;

typedef int                  Boolean;
typedef signed long long int Integer;

typedef unsigned long long int Size;
typedef signed long long int   Difference;

typedef unsigned int Index;
typedef signed int   Count;

typedef long long Address;
typedef long long Handle;

#endif
