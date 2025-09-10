/**
 * @file platform_defines.h
 * @brief Comprehensive platform detection header.
 *
 * This header provides a robust set of preprocessor macros to identify
 * the operating system, processor architecture, endianness, compiler,
 * instruction set modes, ABI, object file format, fundamental
 * type sizes, SIMD instruction set support, and Libc type of the build environment.
 * This enables highly portable and adaptive C/C++ code development,
 * particularly for scenarios like Foreign Function Interfaces (FFI)
 * and low-level machine code generation.
 *
 * Usage:
 * Simply include this header in your source files to leverage the defined macros.
 *
 * Example:
 * #if defined(PLATFORM_OS_LINUX) && defined(PLATFORM_ARCH_AMD64)
 * // Code specific to 64-bit Linux systems
 * #endif
 *
 * #if defined(PLATFORM_COMPILER_MSVC)
 * // Microsoft Visual C++ specific code
 * #endif
 *
 * #if defined(PLATFORM_ARCH_ARM_HARDFLOAT)
 * // Code optimized for ARM hard-float ABI
 * #endif
 *
 * #if (PLATFORM_SIZEOF_POINTER == 8)
 * // Code for 64-bit pointer environments
 * #endif
 *
 * #if defined(PLATFORM_CALL_CONV_SYSV_AMD64)
 * // Code adhering to System V AMD64 ABI calling convention
 * #endif
 *
 * #if defined(PLATFORM_SYSCALL_LINUX_X86_64)
 * // Code for Linux x86-64 syscalls
 * #endif
 *
 * #if defined(PLATFORM_SIMD_AVX2)
 * // Code optimized for AVX2 instruction set
 * #endif
 *
 * #if defined(PLATFORM_LIBC_GLIBC)
 * // Code specific to GNU C Library
 * #endif
 *
 * #if defined(PLATFORM_HAS_BOOL)
 * // Code that uses the _Bool or bool type
 * #endif
 */

#pragma once // Ensures this header is included only once per compilation unit

// Standard C/C++ library includes for type size information
#include <stddef.h> // Provides size_t, ptrdiff_t
#include <limits.h> // Provides CHAR_BIT

// Include for Apple-specific platform targeting (e.g., iOS vs. macOS)
#if defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#endif

// ----------------------------------------------------------------------------
// Section 1: Operating System Detection
// Defines macros for the operating system family and specific variants.
// ----------------------------------------------------------------------------

// These macros are defined if the corresponding OS is detected.
// Check for their presence using #ifdef.

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_OS_WINDOWS 1
    // Include for Windows version detection (if not already defined by build system)
    #ifndef _WIN32_WINNT
        // Default to Windows 7 (0x0601) or later if not explicitly defined to get modern APIs.
        // This ensures access to a reasonable set of Windows APIs.
        #define _WIN32_WINNT 0x0601
    #endif
    // Windows Version Detection based on _WIN32_WINNT and specific variants
    #if defined(_M_ARM64EC)
        #define PLATFORM_OS_WINDOWS_ARM64EC 1
        #define PLATFORM_OS_NAME "Windows on ARM64EC"
    #elif defined(_M_ARM64)
        #define PLATFORM_OS_WINDOWS_ARM64 1
        #define PLATFORM_OS_NAME "Windows on ARM (64-bit)"
    #elif defined(_M_ARM)
        #define PLATFORM_OS_WINDOWS_ARM32 1
        #define PLATFORM_OS_NAME "Windows on ARM (32-bit)"
    #elif defined(__CYGWIN__)
        #define PLATFORM_OS_CYGWIN 1
        #define PLATFORM_OS_NAME "Cygwin (Windows)"
    #elif defined(__MINGW32__)
        #define PLATFORM_OS_MINGW 1
        #define PLATFORM_OS_NAME "MinGW (Windows)"
    #elif (_WIN32_WINNT >= 0x0A00) && defined(_WIN32_WINNT_WIN11) // This macro is not standard, but for future proofing
        #define PLATFORM_OS_WINDOWS_11_OR_LATER 1
        #define PLATFORM_OS_NAME "Windows 11+"
    #elif (_WIN32_WINNT >= 0x0A00) // Windows 10 (10.0) and later
        #define PLATFORM_OS_WINDOWS_10_OR_LATER 1
        #define PLATFORM_OS_NAME "Windows 10+"
    #elif (_WIN32_WINNT >= 0x0603) // Windows 8.1 (6.3)
        #define PLATFORM_OS_WINDOWS_8_1_OR_LATER 1
        #define PLATFORM_OS_NAME "Windows 8.1+"
    #elif (_WIN32_WINNT >= 0x0602) // Windows 8 (6.2)
        #define PLATFORM_OS_WINDOWS_8_OR_LATER 1
        #define PLATFORM_OS_NAME "Windows 8+"
    #elif (_WIN32_WINNT >= 0x0601) // Windows 7 (6.1)
        #define PLATFORM_OS_WINDOWS_7_OR_LATER 1
        #define PLATFORM_OS_NAME "Windows 7+"
    #elif (_WIN32_WINNT >= 0x0600) // Windows Vista (6.0)
        #define PLATFORM_OS_WINDOWS_VISTA_OR_LATER 1
        #define PLATFORM_OS_NAME "Windows Vista+"
    #elif (_WIN32_WINNT >= 0x0501) // Windows XP (5.1)
        #define PLATFORM_OS_WINDOWS_XP_OR_LATER 1
        #define PLATFORM_OS_NAME "Windows XP+"
    #else
        #define PLATFORM_OS_NAME "Windows" // Generic Windows if no specific version/variant matched
    #endif

#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_OS_APPLE 1
    #if TARGET_OS_IPHONE
        #define PLATFORM_OS_IOS 1
        #define PLATFORM_OS_NAME "iOS"
    #elif TARGET_OS_TV
        #define PLATFORM_OS_TVOS 1
        #define PLATFORM_OS_NAME "tvOS"
    #elif TARGET_OS_WATCH
        #define PLATFORM_OS_WATCHOS 1
        #define PLATFORM_OS_NAME "watchOS"
    #elif TARGET_OS_MAC
        #define PLATFORM_OS_MACOS 1
        #if defined(__aarch64__)
            #define PLATFORM_OS_MACOS_ARM64 1
            #define PLATFORM_OS_NAME "macOS (Apple Silicon)"
        #elif defined(__x86_64__)
            #define PLATFORM_OS_MACOS_AMD64 1
            #define PLATFORM_OS_NAME "macOS (Intel 64-bit)"
        #elif defined(__i386__)
            #define PLATFORM_OS_MACOS_I86 1
            #define PLATFORM_OS_NAME "macOS (Intel 32-bit)"
        #else
            #define PLATFORM_OS_NAME "macOS" // Generic macOS if no specific arch matched
        #endif
    #else
        #define PLATFORM_OS_NAME "Apple OS" // Generic Apple OS, if no specific variant matched
    #endif
#elif defined(__linux__)
    #define PLATFORM_OS_LINUX 1
    #if defined(__ANDROID__)
        #define PLATFORM_OS_ANDROID 1
        #define PLATFORM_OS_NAME "Android" // Android is Linux-based
        // Termux is a user-space environment on Android.
        // It's not reliably detectable via standard compile-time preprocessor macros.
        // Runtime checks would be needed (e.g., checking environment variables like TERMUX_VERSION
        // or file paths like /data/data/com.termux).
    #elif defined(__WSL__) // Non-standard, but sometimes used by build systems or custom toolchains
        #define PLATFORM_OS_WSL 1
        #define PLATFORM_OS_NAME "WSL (Windows Subsystem for Linux)"
    #else
        #define PLATFORM_OS_NAME "Linux"
        // Crostini (Chrome OS Linux container) detection:
        // Similar to WSL, compiling inside Crostini will identify as Linux.
        // Compile-time detection is not standard. Runtime checks are typical
        // (e.g., checking /etc/os-release for `ID="debian"` and `VARIANT_ID="cros"`).
        // Docker Detection:
        // Detecting if code is running inside a Docker container is primarily a runtime concern,
        // not a compile-time one that can be determined by standard C preprocessor macros.
        // It usually involves checking `/proc/self/cgroup` or environment variables at runtime.
        // Note: Specific Linux distributions (Ubuntu, Fedora, etc.) are typically
        // detected at the build system level (e.g., CMake, Autotools) or runtime,
        // as there are no universal preprocessor macros for them.
    #endif

#elif defined(__FreeBSD__)
    #define PLATFORM_OS_FREEBSD 1
    #define PLATFORM_OS_NAME "FreeBSD"
#elif defined(__OpenBSD__)
    #define PLATFORM_OS_OPENBSD 1
    #define PLATFORM_OS_NAME "OpenBSD"
#elif defined(__NetBSD__)
    #define PLATFORM_OS_NETBSD 1
    #define PLATFORM_OS_NAME "NetBSD"
#elif defined(__DragonFly__)
    #define PLATFORM_OS_DRAGONFLYBSD 1
    #define PLATFORM_OS_NAME "DragonFly BSD"
#elif defined(__sun) && defined(__SVR4)
    #define PLATFORM_OS_SOLARIS 1
    #define PLATFORM_OS_NAME "Solaris"
#elif defined(__EMSCRIPTEN__)
    #define PLATFORM_OS_EMSCRIPTEN 1
    #define PLATFORM_OS_NAME "Emscripten (WebAssembly)"
#elif defined(_AIX)
    #define PLATFORM_OS_AIX 1
    #define PLATFORM_OS_NAME "AIX"
#elif defined(__amigaos__)
    #define PLATFORM_OS_AMIGAOS 1
    #define PLATFORM_OS_NAME "AmigaOS"
#elif defined(__BS2000__)
    #define PLATFORM_OS_BS2000 1
    #define PLATFORM_OS_NAME "POSIX-BC BS2000"
#elif defined(__HAIKU__)
    #define PLATFORM_OS_HAIKU 1
    #define PLATFORM_OS_NAME "Haiku"
#elif defined(__BEOS__) // Older BeOS
    #define PLATFORM_OS_BEOS 1
    #define PLATFORM_OS_NAME "BeOS"
#elif defined(__hpux)
    #define PLATFORM_OS_HPUX 1
    #define PLATFORM_OS_NAME "HP-UX"
#elif defined(__GNU__) && !defined(__linux__)
    // __GNU__ is often defined with __linux__ for GNU/Linux.
    // If __linux__ is NOT defined, it's likely GNU Hurd.
    #define PLATFORM_OS_HURD 1
    #define PLATFORM_OS_NAME "GNU Hurd"
#elif defined(__sgi) || defined(sgi) // Older Irix compilers might use 'sgi'
    #define PLATFORM_OS_IRIX 1
    #define PLATFORM_OS_NAME "IRIX"
#elif defined(__minix)
    #define PLATFORM_OS_MINIX 1
    #define PLATFORM_OS_NAME "Minix"
#elif defined(__OS2__)
    #define PLATFORM_OS_OS2 1
    #define PLATFORM_OS_NAME "OS/2"
#elif defined(__MVS__) || defined(__zos__)
    #define PLATFORM_OS_ZOS 1
    #define PLATFORM_OS_NAME "z/OS (OS/390)"
#elif defined(__OS400__)
    #define PLATFORM_OS_OS400 1
    #define PLATFORM_OS_NAME "OS/400 (IBM i)"
#elif defined(_PLAN9)
    #define PLATFORM_OS_PLAN9 1
    #define PLATFORM_OS_NAME "Plan 9"
#elif defined(__QNX__) || defined(__QNXNTO__)
    #define PLATFORM_OS_QNX 1
    #define PLATFORM_OS_NAME "QNX"
#elif defined(__riscos__)
    #define PLATFORM_OS_RISCOS 1
    #define PLATFORM_OS_NAME "RISC OS"
#elif defined(__osf__)
    #define PLATFORM_OS_TRU64 1
    #define PLATFORM_OS_NAME "Tru64 UNIX"
#elif defined(__VMS)
    #define PLATFORM_OS_VMS 1
    #define PLATFORM_OS_NAME "OpenVMS"
#elif defined(__VOS__)
    #define PLATFORM_OS_VOS 1
    #define PLATFORM_OS_NAME "Stratus VOS"
#elif defined(__vxworks)
    #define PLATFORM_OS_VXWORKS 1
    #define PLATFORM_OS_NAME "VxWorks"
#elif defined(__unix__) || defined(__unix)
    // Generic Unix-like fallback for systems not explicitly listed above
    #define PLATFORM_OS_UNIX 1
    #define PLATFORM_OS_NAME "Generic Unix-like"
#else
    #define PLATFORM_OS_NAME "Unknown OS"
#endif

// ----------------------------------------------------------------------------
// Section 2: Processor Architecture Detection
// Defines macros for the CPU architecture, bitness, instruction set modes, and ABI.
// ----------------------------------------------------------------------------

// These macros are defined if the corresponding architecture/feature is detected.
// Check for their presence using #ifdef.

#if defined(__aarch64__) || defined(_M_ARM64)
    #define PLATFORM_ARCH_ARM64 1
    #define PLATFORM_ARCH_BITS 64
    #define PLATFORM_ARCH_ABI_EABI 1
    #define PLATFORM_ARCH_ABI_NAME "EABI"
    #if defined(_M_ARM64EC)
        #define PLATFORM_ARCH_ARM64EC 1
        #define PLATFORM_ARCH_NAME "ARM64EC"
    #else
        #define PLATFORM_ARCH_NAME "ARM64 (AArch64)"
    #endif
    // ARM64 typically uses hard-float by default
    #define PLATFORM_ARCH_ARM64_HARDFLOAT 1
#elif defined(__arm__) || defined(_M_ARM)
    #define PLATFORM_ARCH_ARM 1
    #define PLATFORM_ARCH_BITS 32
    #if defined(__ARM_EABI__)
        #define PLATFORM_ARCH_ARM_ABI_EABI 1
        #define PLATFORM_ARCH_ABI_NAME "EABI"
    #else
        #define PLATFORM_ARCH_ARM_ABI_OABI 1
        #define PLATFORM_ARCH_ABI_NAME "OABI (or old ABI)"
    #endif
    // ARM Instruction Set Mode (ARM vs. Thumb)
    #if defined(__ARM_ARCH_ISA_ARM)
        #define PLATFORM_ARCH_ARM_MODE_ARM 1
        #define PLATFORM_ARCH_NAME "ARM (32-bit, ARM Mode)"
    #elif defined(__ARM_ARCH_ISA_THUMB)
        #define PLATFORM_ARCH_ARM_MODE_THUMB 1
        #define PLATFORM_ARCH_NAME "ARM (32-bit, Thumb Mode)"
    #else
        #define PLATFORM_ARCH_NAME "ARM (32-bit)"
    #endif
    // ARM Floating Point ABI detection
    #if defined(__ARM_PCS_VFP) || defined(__VFP_FP__) // __VFP_FP__ common for hard-float
        #define PLATFORM_ARCH_ARM_HARDFLOAT 1
    #else
        #define PLATFORM_ARCH_ARM_SOFTFLOAT 1
    #endif
#elif defined(__x86_64__) || defined(_M_X64)
    #define PLATFORM_ARCH_AMD64 1
    #define PLATFORM_ARCH_NAME "AMD64 (x86 64-bit)"
    #define PLATFORM_ARCH_BITS 64
    #define PLATFORM_ARCH_ABI_NAME "System V AMD64 or Microsoft x64"
#elif defined(__i386__) || defined(_M_IX86)
    #define PLATFORM_ARCH_I86 1
    #define PLATFORM_ARCH_NAME "i86 (x86 32-bit)"
    #define PLATFORM_ARCH_BITS 32
    #define PLATFORM_ARCH_ABI_NAME "System V i386 or Microsoft x86"
#elif defined(__mips__) || defined(__mips) || defined(_MIPS_ARCH)
    #define PLATFORM_ARCH_MIPS 1
    #define PLATFORM_ARCH_NAME "MIPS"
    #if defined(__LP64__) || defined(_MIPS_ARCH_64)
        #define PLATFORM_ARCH_BITS 64
    #else
        #define PLATFORM_ARCH_BITS 32
    #endif
#elif defined(__powerpc__) || defined(__ppc__) || defined(_M_PPC)
    #define PLATFORM_ARCH_POWERPC 1
    #define PLATFORM_ARCH_NAME "PowerPC"
    #if defined(__PPC64__)
        #define PLATFORM_ARCH_BITS 64
    #else
        #define PLATFORM_ARCH_BITS 32
    #endif
#elif defined(__sparc__)
    #define PLATFORM_ARCH_SPARC 1
    #define PLATFORM_ARCH_NAME "SPARC"
    #if defined(__LP64__)
        #define PLATFORM_ARCH_BITS 64
    #else
        #define PLATFORM_ARCH_BITS 32
    #endif
#elif defined(__riscv)
    #define PLATFORM_ARCH_RISCV 1
    #define PLATFORM_ARCH_NAME "RISC-V"
    #if __riscv_xlen == 64
        #define PLATFORM_ARCH_BITS 64
    #else
        #define PLATFORM_ARCH_BITS 32
    #endif
#else
    #define PLATFORM_ARCH_NAME "Unknown Architecture"
    #define PLATFORM_ARCH_BITS 0
    #define PLATFORM_ARCH_ABI_NAME "Unknown ABI"
#endif // End of Processor Architecture Detection

// ----------------------------------------------------------------------------
// Section 3: Endianness Detection
// Determines if the system is little-endian or big-endian.
// ----------------------------------------------------------------------------

// These macros are defined if the corresponding endianness is detected.
// Check for their presence using #ifdef.

// GCC/Clang built-in macros are most reliable
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    #define PLATFORM_ENDIAN_LITTLE 1
    #define PLATFORM_ENDIAN_NAME "Little-endian"
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #define PLATFORM_ENDIAN_BIG 1
    #define PLATFORM_ENDIAN_NAME "Big-endian"
#elif defined(_WIN32) || defined(_WIN64)
    // Windows is almost exclusively little-endian on x86/x64/ARM
    #define PLATFORM_ENDIAN_LITTLE 1
    #define PLATFORM_ENDIAN_NAME "Little-endian"
#else
    // Fallback for other compilers/platforms.
    // This is less reliable and might require a runtime check if absolute
    // certainty is needed. Most modern systems are little-endian.
    #warning "Could not reliably determine endianness at compile time. Assuming little-endian."
    #define PLATFORM_ENDIAN_LITTLE 1
    #define PLATFORM_ENDIAN_NAME "Assumed Little-endian"
    // You can add a runtime check for endianness if this fallback is insufficient:
    // static const union { unsigned int i; char c[4]; } _test_endian = {0x01020304};
    // #define IS_LITTLE_ENDIAN (_test_endian.c[0] == 0x04)
    // #define IS_BIG_ENDIAN (_test_endian.c[0] == 0x01)
#endif

// ----------------------------------------------------------------------------
// Section 4: Compiler Detection
// Identifies the C/C++ compiler being used.
// ----------------------------------------------------------------------------

// These macros are defined if the corresponding compiler is detected.
// Check for their presence using #ifdef.

#if defined(__clang__)
    #define PLATFORM_COMPILER_CLANG 1
    #define PLATFORM_COMPILER_NAME "Clang"
    #define PLATFORM_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
    #define PLATFORM_COMPILER_GCC 1
    #define PLATFORM_COMPILER_NAME "GCC"
    #define PLATFORM_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER)
    #define PLATFORM_COMPILER_MSVC 1
    #define PLATFORM_COMPILER_NAME "MSVC"
    #define PLATFORM_COMPILER_VERSION _MSC_VER
#elif defined(__INTEL_COMPILER) || defined(__ICC) || defined(__ECC)
    #define PLATFORM_COMPILER_INTEL 1
    #define PLATFORM_COMPILER_NAME "Intel C/C++"
    #define PLATFORM_COMPILER_VERSION __INTEL_COMPILER
#elif defined(__TINYC__)
    #define PLATFORM_COMPILER_TCC 1
    #define PLATFORM_COMPILER_NAME "Tiny C Compiler"
    #define PLATFORM_COMPILER_VERSION __TINYC__
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
    #define PLATFORM_COMPILER_SUNPRO 1
    #define PLATFORM_COMPILER_NAME "Oracle Solaris Studio"
    #define PLATFORM_COMPILER_VERSION __SUNPRO_C
#elif defined(__HP_cc) || defined(__HP_aCC)
    #define PLATFORM_COMPILER_HP 1
    #define PLATFORM_COMPILER_NAME "HP C/aC++"
    #define PLATFORM_COMPILER_VERSION __HP_cc
#elif defined(__IBM_GCC_ASM) || defined(__xlC__) || defined(__IBMC__)
    #define PLATFORM_COMPILER_IBM 1
    #define PLATFORM_COMPILER_NAME "IBM XL C/C++"
    #define PLATFORM_COMPILER_VERSION __xlC__
#else
    #define PLATFORM_COMPILER_NAME "Unknown Compiler"
    #define PLATFORM_COMPILER_VERSION 0
#endif

// ----------------------------------------------------------------------------
// Section 5: Object File Format Detection
// Identifies the executable/object file format used by the target system.
// ----------------------------------------------------------------------------

// These macros are defined if the corresponding object format is detected.
// Check for their presence using #ifdef.

#if defined(__ELF__)
    #define PLATFORM_OBJECT_FORMAT_ELF 1
    #if defined(PLATFORM_ARCH_BITS) && (PLATFORM_ARCH_BITS == 64)
        #define PLATFORM_OBJECT_FORMAT_ELF64 1
        #define PLATFORM_OBJECT_FORMAT_NAME "ELF64"
    #else
        #define PLATFORM_OBJECT_FORMAT_ELF32 1
        #define PLATFORM_OBJECT_FORMAT_NAME "ELF32"
    #endif
#elif defined(__APPLE__)
    // Apple uses Mach-O for executables and object files
    #define PLATFORM_OBJECT_FORMAT_MACH_O 1
    #define PLATFORM_OBJECT_FORMAT_NAME "Mach-O"
#elif defined(_WIN32) || defined(_WIN64)
    // Windows uses Portable Executable (PE) format for executables
    // and COFF for object files, but PE is the primary target format.
    #define PLATFORM_OBJECT_FORMAT_PE 1
    #define PLATFORM_OBJECT_FORMAT_NAME "PE (Portable Executable)"
#else
    #define PLATFORM_OBJECT_FORMAT_NAME "Unknown Object Format"
#endif

// ----------------------------------------------------------------------------
// Section 6: Core ABI & Low-Level Details for FFI
// Provides details relevant for machine code generation and Foreign Function Interfaces (FFI).
// ----------------------------------------------------------------------------

// Initialize with default/unknown values, then override based on OS/Arch
#define PLATFORM_STACK_ALIGNMENT 0
#define PLATFORM_NUM_INT_ARG_REGS 0
#define PLATFORM_NUM_FLOAT_ARG_REGS 0
// #define PLATFORM_CALLER_CLEANS_STACK 1 // Defined if caller cleans stack, else callee cleans

// Windows x86/x64 Calling Conventions
#if defined(PLATFORM_OS_WINDOWS)
    #if defined(PLATFORM_ARCH_AMD64) // 64-bit Windows
        // x64 calling convention (Microsoft x64 calling convention):
        // RCX, RDX, R8, R9 for integer/pointer args. XMM0-XMM3 for float/double.
        // Stack space (shadow space) reserved by caller. Caller cleans stack.
        #define PLATFORM_CALL_CONV_MSVC_X64 1
        #undef PLATFORM_STACK_ALIGNMENT
        #define PLATFORM_STACK_ALIGNMENT 16        // 16-byte stack alignment
        #undef PLATFORM_NUM_INT_ARG_REGS
        #define PLATFORM_NUM_INT_ARG_REGS 4        // RCX, RDX, R8, R9
        #undef PLATFORM_NUM_FLOAT_ARG_REGS
        #define PLATFORM_NUM_FLOAT_ARG_REGS 4      // XMM0, XMM1, XMM2, XMM3
        #define PLATFORM_CALLER_CLEANS_STACK 1
        #define PLATFORM_SYSCALL_WINDOWS_AMD64 1   // Syscall: 'syscall' instruction
    #elif defined(PLATFORM_ARCH_I86) // 32-bit Windows
        #define PLATFORM_CALL_CONV_MSVC_CDECL 1    // __cdecl: Caller cleans stack, args right-to-left on stack
        #define PLATFORM_CALL_CONV_MSVC_STDCALL 1  // __stdcall: Callee cleans stack, args right-to-left on stack (WinAPI default)
        #define PLATFORM_CALL_CONV_MSVC_FASTCALL 1 // __fastcall: First few args in registers (ECX, EDX), rest on stack. Callee cleans.
        #define PLATFORM_CALL_CONV_MSVC_THISCALL 1 // __thiscall: For C++ member functions, 'this' in ECX. Callee cleans.
        #define PLATFORM_SYSCALL_WINDOWS_I86 1     // Syscall: int 2Eh (or sysenter/syscall for kernel mode)
        #undef PLATFORM_STACK_ALIGNMENT
        #define PLATFORM_STACK_ALIGNMENT 4         // 4-byte stack alignment for 32-bit Windows
        // No fixed integer/float argument registers for stack-based conventions, or they are few and specific to __fastcall/__thiscall
        // For FFI, assume stack for general purpose if not explicitly fastcall.
    #elif defined(PLATFORM_ARCH_ARM32) || defined(PLATFORM_ARCH_ARM64) // Windows on ARM
        // ARM EABI-like convention for Windows ARM.
        // R0-R3 for integer args, S0-S15/D0-D7 for float/double. Caller cleans.
        #define PLATFORM_CALL_CONV_MSVC_ARM 1
        #undef PLATFORM_STACK_ALIGNMENT
        #define PLATFORM_STACK_ALIGNMENT 8         // 8-byte stack alignment (or 16 for some contexts)
        #undef PLATFORM_NUM_INT_ARG_REGS
        #define PLATFORM_NUM_INT_ARG_REGS 4        // R0-R3
        #undef PLATFORM_NUM_FLOAT_ARG_REGS
        #define PLATFORM_NUM_FLOAT_ARG_REGS 8      // D0-D7 (for double)
        #define PLATFORM_CALLER_CLEANS_STACK 1
        #define PLATFORM_SYSCALL_WINDOWS_ARM 1     // Syscall: Varies, often through specific kernel functions or SVC instruction
    #endif
#endif // PLATFORM_OS_WINDOWS

// Unix-like (Linux, macOS, BSDs) Calling Conventions
#if defined(PLATFORM_OS_LINUX) || defined(PLATFORM_OS_APPLE) || defined(PLATFORM_OS_UNIX) || \
    defined(PLATFORM_OS_FREEBSD) || defined(PLATFORM_OS_OPENBSD) || defined(PLATFORM_OS_NETBSD) || \
    defined(PLATFORM_OS_DRAGONFLYBSD) || defined(PLATFORM_OS_SOLARIS) || defined(PLATFORM_OS_AIX) || \
    defined(PLATFORM_OS_QNX) || defined(PLATFORM_OS_HPUX) || defined(PLATFORM_OS_IRIX) || \
    defined(PLATFORM_OS_HURD) || defined(PLATFORM_OS_ANDROID) || defined(PLATFORM_OS_EMSCRIPTEN) || \
    defined(PLATFORM_OS_MINIX) || defined(PLATFORM_OS_VXWORKS)

    #define PLATFORM_CALLER_CLEANS_STACK 1 // Most Unix-like ABIs are cdecl-like (caller cleans)

    #if defined(PLATFORM_ARCH_AMD64) // 64-bit Unix-like (Linux x86-64, macOS x86-64)
        // System V AMD64 ABI:
        // RDI, RSI, RDX, RCX, R8, R9 for integer/pointer args.
        // XMM0-XMM7 for float/double args.
        // Stack space not reserved. Caller cleans stack.
        #define PLATFORM_CALL_CONV_SYSV_AMD64 1
        #undef PLATFORM_STACK_ALIGNMENT
        #define PLATFORM_STACK_ALIGNMENT 16 // 16-byte stack alignment
        #undef PLATFORM_NUM_INT_ARG_REGS
        #define PLATFORM_NUM_INT_ARG_REGS 6 // RDI, RSI, RDX, RCX, R8, R9
        #undef PLATFORM_NUM_FLOAT_ARG_REGS
        #define PLATFORM_NUM_FLOAT_ARG_REGS 8 // XMM0-XMM7
        // Syscall: 'syscall' instruction
        #if defined(PLATFORM_OS_LINUX) || defined(PLATFORM_OS_ANDROID) || defined(PLATFORM_OS_WSL)
            #define PLATFORM_SYSCALL_LINUX_X86_64 1
        #elif defined(PLATFORM_OS_MACOS)
            #define PLATFORM_SYSCALL_MACOS_AMD64 1 // syscall instruction (differs from Linux)
        #endif
    #elif defined(PLATFORM_ARCH_I86) // 32-bit Unix-like (e.g., Linux i386, macOS i386)
        // System V i386 ABI (cdecl-like): All args on stack, right-to-left. Caller cleans.
        #define PLATFORM_CALL_CONV_SYSV_I86 1
        #undef PLATFORM_STACK_ALIGNMENT
        #define PLATFORM_STACK_ALIGNMENT 16 // Common, though 4-byte is base. GCC often aligns to 16.
        // No fixed integer/float argument registers for stack-based conventions
        // Syscall: int 0x80 (Linux), or specific trap/syscall instructions
        #if defined(PLATFORM_OS_LINUX) || defined(PLATFORM_OS_ANDROID) || defined(PLATFORM_OS_WSL) || defined(PLATFORM_OS_MINIX)
            #define PLATFORM_SYSCALL_LINUX_I86 1 // int 0x80
        #endif
        #if defined(PLATFORM_OS_MACOS) // macOS 32-bit
            #define PLATFORM_SYSCALL_MACOS_I86 1 // Trap-based
        #endif
    #elif defined(PLATFORM_ARCH_ARM64) // 64-bit ARM Unix-like (e.g., Linux AArch64, macOS Apple Silicon)
        // AArch64 ABI (part of AAPCS)
        // X0-X7 for integer/pointer args. V0-V7 for float/double.
        // Stack aligned to 16 bytes. Caller cleans stack.
        #define PLATFORM_CALL_CONV_AAPCS64 1
        #undef PLATFORM_STACK_ALIGNMENT
        #define PLATFORM_STACK_ALIGNMENT 16
        #undef PLATFORM_NUM_INT_ARG_REGS
        #define PLATFORM_NUM_INT_ARG_REGS 8 // X0-X7
        #undef PLATFORM_NUM_FLOAT_ARG_REGS
        #define PLATFORM_NUM_FLOAT_ARG_REGS 8 // V0-V7
        // Syscall: SVC instruction
        #if defined(PLATFORM_OS_LINUX) || defined(PLATFORM_OS_ANDROID) || defined(PLATFORM_OS_WSL)
            #define PLATFORM_SYSCALL_LINUX_ARM64 1
        #elif defined(PLATFORM_OS_MACOS)
            #define PLATFORM_SYSCALL_MACOS_ARM64 1 // SVC instruction (differs from Linux)
        #endif
    #elif defined(PLATFORM_ARCH_ARM) // 32-bit ARM Unix-like (e.g., Linux ARM, Android ARM)
        // ARM EABI (AAPCS - ARM Architecture Procedure Call Standard)
        // R0-R3 for integer/pointer args. S0-S15/D0-D7 for float/double (hard-float).
        // Stack aligned to 8 bytes. Caller cleans stack.
        #define PLATFORM_CALL_CONV_AAPCS32 1
        #undef PLATFORM_STACK_ALIGNMENT
        #define PLATFORM_STACK_ALIGNMENT 8
        #undef PLATFORM_NUM_INT_ARG_REGS
        #define PLATFORM_NUM_INT_ARG_REGS 4 // R0-R3
        #if defined(PLATFORM_ARCH_ARM_HARDFLOAT)
            #undef PLATFORM_NUM_FLOAT_ARG_REGS
            #define PLATFORM_NUM_FLOAT_ARG_REGS 8 // D0-D7 for double
        #else
            #undef PLATFORM_NUM_FLOAT_ARG_REGS
            #define PLATFORM_NUM_FLOAT_ARG_REGS 0 // Soft-float uses integer registers/stack
        #endif
        // Syscall: SVC instruction
        #if defined(PLATFORM_OS_LINUX) || defined(PLATFORM_OS_ANDROID) || defined(PLATFORM_OS_WSL) || defined(PLATFORM_OS_VXWORKS)
            #define PLATFORM_SYSCALL_LINUX_ARM 1
        #endif
    #endif
#endif // End of Unix-like Calling Conventions

// Other specific OS/Arch calling conventions
#if defined(PLATFORM_OS_VMS)
    // VMS has its own unique calling conventions, often involving descriptors.
    // Too complex for generic preprocessor defines, requires specific VMS headers/libraries.
    #define PLATFORM_CALL_CONV_VMS_SPECIFIC 1
#endif


// ----------------------------------------------------------------------------
// Section 7: Hardware Capabilities (SIMD)
// Detects support for various Single Instruction, Multiple Data (SIMD) extensions.
// ----------------------------------------------------------------------------

// These macros are defined if the corresponding SIMD instruction set is supported
// by the target architecture and enabled by the compiler.
// Check for their presence using #ifdef.

#if defined(PLATFORM_ARCH_I86) || defined(PLATFORM_ARCH_AMD64) // x86/x64 SIMD
    #if defined(__MMX__)
        #define PLATFORM_SIMD_MMX 1
    #endif
    #if defined(__SSE__)
        #define PLATFORM_SIMD_SSE 1
    #endif
    #if defined(__SSE2__)
        #define PLATFORM_SIMD_SSE2 1
    #endif
    #if defined(__SSE3__)
        #define PLATFORM_SIMD_SSE3 1
    #endif
    #if defined(__SSSE3__)
        #define PLATFORM_SIMD_SSSE3 1
    #endif
    #if defined(__SSE4_1__)
        #define PLATFORM_SIMD_SSE4_1 1
    #endif
    #if defined(__SSE4_2__)
        #define PLATFORM_SIMD_SSE4_2 1
    #endif
    #if defined(__AVX__)
        #define PLATFORM_SIMD_AVX 1
    #endif
    #if defined(__AVX2__)
        #define PLATFORM_SIMD_AVX2 1
    #endif
    #if defined(__AVX512F__) // AVX-512 Foundation
        #define PLATFORM_SIMD_AVX512F 1
    #endif
    #if defined(__AVX512VL__) // AVX-512 Vector Length Extensions
        #define PLATFORM_SIMD_AVX512VL 1
    #endif
    #if defined(__AVX512BW__) // AVX-512 Byte and Word Instructions
        #define PLATFORM_SIMD_AVX512BW 1
    #endif
    #if defined(__AVX512DQ__) // AVX-512 Doubleword and Quadword Instructions
        #define PLATFORM_SIMD_AVX512DQ 1
    #endif
    #if defined(__AVX512CD__) // AVX-512 Conflict Detection Instructions
        #define PLATFORM_SIMD_AVX512CD 1
    #endif
    #if defined(__AVX512ER__) // AVX-512 Exponential and Reciprocal Instructions
        #define PLATFORM_SIMD_AVX512ER 1
    #endif
    #if defined(__AVX512PF__) // AVX-512 PreFetch Instructions
        #define PLATFORM_SIMD_AVX512PF 1
    #endif
    #if defined(__AVX512VBMI__) // AVX-512 Vector Bit Manipulation Instructions
        #define PLATFORM_SIMD_AVX512VBMI 1
    #endif
#endif // x86/x64 SIMD

#if defined(PLATFORM_ARCH_ARM) // 32-bit ARM SIMD
    #if defined(__ARM_NEON__) || defined(__ARM_NEON)
        #define PLATFORM_SIMD_NEON 1
    #endif
#endif // 32-bit ARM SIMD

#if defined(PLATFORM_ARCH_ARM64) // ARM64 (AArch64) SIMD
    // ASIMD (Advanced SIMD) is the instruction set for AArch64, equivalent to NEON on 32-bit ARM.
    // The __ARM_NEON__ macro is often also defined for AArch64 when NEON is available.
    #if defined(__ARM_NEON__) || defined(__ARM_NEON)
        #define PLATFORM_SIMD_ASIMD 1 // Explicitly define ASIMD for AArch64
        #define PLATFORM_SIMD_NEON 1  // Also define NEON for compatibility/generality
    #endif
#endif // ARM64 (AArch64) SIMD

#if defined(PLATFORM_ARCH_POWERPC) // PowerPC SIMD
    #if defined(__ALTIVEC__) || defined(__VEC__)
        #define PLATFORM_SIMD_ALTIVEC 1
    #endif
#endif // PowerPC SIMD

#if defined(PLATFORM_ARCH_MIPS) // MIPS SIMD
    #if defined(__mips_msa)
        #define PLATFORM_SIMD_MSA 1
    #endif
#endif // MIPS SIMD


// ----------------------------------------------------------------------------
// Section 8: Software Environment (Libc)
// Identifies the C standard library implementation being used.
// ----------------------------------------------------------------------------

// These macros are defined if the corresponding libc is detected.
// Check for their presence using #ifdef.

#if defined(__GLIBC__) || defined(__GNU_LIBRARY__)
    #define PLATFORM_LIBC_GLIBC 1
    #define PLATFORM_LIBC_NAME "GNU C Library (glibc)"
    #define PLATFORM_LIBC_VERSION_MAJOR __GLIBC__
    #define PLATFORM_LIBC_VERSION_MINOR __GLIBC_MINOR__
#elif defined(__MUSL__)
    #define PLATFORM_LIBC_MUSL 1
    #define PLATFORM_LIBC_NAME "musl libc"
    // musl does not typically expose version macros like glibc.
#elif defined(__BIONIC__)
    #define PLATFORM_LIBC_BIONIC 1
    #define PLATFORM_LIBC_NAME "Bionic (Android)"
#elif defined(__UCLIBC__)
    #define PLATFORM_LIBC_UCLIBC 1
    #define PLATFORM_LIBC_NAME "uClibc"
    #define PLATFORM_LIBC_VERSION_MAJOR __UCLIBC_MAJOR__
    #define PLATFORM_LIBC_VERSION_MINOR __UCLIBC_MINOR__
#elif defined(__NEWLIB__)
    #define PLATFORM_LIBC_NEWLIB 1
    #define PLATFORM_LIBC_NAME "Newlib"
#elif defined(_MSC_VER)
    // On Windows with MSVC, the C runtime is part of the Visual C++ Redistributable.
    // It's often referred to as MSVCRT or Universal C Runtime (UCRT).
    #define PLATFORM_LIBC_MSVCRT 1
    #define PLATFORM_LIBC_NAME "Microsoft Visual C++ Runtime"
#elif defined(__APPLE__)
    // Apple platforms use their own C standard library, which is largely POSIX-compliant
    // but not glibc, musl, etc. It's based on FreeBSD's libc.
    #define PLATFORM_LIBC_APPLE 1
    #define PLATFORM_LIBC_NAME "Apple Libc"
#else
    #define PLATFORM_LIBC_NAME "Unknown Libc"
#endif

// ----------------------------------------------------------------------------
// Section 9: Fundamental Type Sizes and Alignment
// Defines properties derived from previous detections and standard type sizes,
// including information relevant to data alignment.
// ----------------------------------------------------------------------------

// CHAR_BIT is defined in <limits.h> and is usually 8.
// Define a fallback if it's somehow not available.
#ifndef CHAR_BIT
    #define CHAR_BIT 8
#endif

// Pointer size (in bytes)
#if defined(PLATFORM_ARCH_BITS) && (PLATFORM_ARCH_BITS == 64)
    #define PLATFORM_SIZEOF_POINTER 8
#elif defined(PLATFORM_ARCH_BITS) && (PLATFORM_ARCH_BITS == 32)
    #define PLATFORM_SIZEOF_POINTER 4
#else
    #warning "Could not reliably determine pointer size at compile time based on architecture. Assuming 8 bytes."
    #define PLATFORM_SIZEOF_POINTER 8 // Default to 64-bit pointer size
#endif

// size_t size (in bytes)
#if defined(PLATFORM_SIZEOF_POINTER) && (PLATFORM_SIZEOF_POINTER != 0)
    #define PLATFORM_SIZEOF_SIZE_T PLATFORM_SIZEOF_POINTER
#elif defined(__SIZEOF_SIZE_T__)
    #define PLATFORM_SIZEOF_SIZE_T __SIZEOF_SIZE_T__
#else
    #warning "Could not reliably determine size_t size at compile time. Assuming 8 bytes."
    #define PLATFORM_SIZEOF_SIZE_T 8 // Default to 64-bit size_t
#endif

// wchar_t size (in bytes)
#if defined(_WIN32) || defined(_WIN64)
    // On Windows, wchar_t is typically 2 bytes (UTF-16)
    #define PLATFORM_SIZEOF_WCHAR_T 2
#elif defined(__SIZEOF_WCHAR_T__)
    // GCC/Clang built-in macro
    #define PLATFORM_SIZEOF_WCHAR_T __SIZEOF_WCHAR_T__
#else
    // Fallback: Assume 4 bytes (UTF-32) on most Unix-like systems,
    // but this can vary.
    #warning "Could not reliably determine wchar_t size at compile time. Assuming 4 bytes."
    #define PLATFORM_SIZEOF_WCHAR_T 4
#endif

// Other common fixed-size type sizes (usually standard, but good to have explicit defines)
#define PLATFORM_SIZEOF_CHAR sizeof(char)
#define PLATFORM_SIZEOF_SHORT sizeof(short)
#define PLATFORM_SIZEOF_INT sizeof(int)
#define PLATFORM_SIZEOF_LONG sizeof(long)
#define PLATFORM_SIZEOF_LONG_LONG sizeof(long long)
#define PLATFORM_SIZEOF_FLOAT sizeof(float)
#define PLATFORM_SIZEOF_DOUBLE sizeof(double)
#define PLATFORM_SIZEOF_LONG_DOUBLE sizeof(long double)

// Maximum alignment required by any fundamental type.
// This is typically the alignment of `long double` or the largest SIMD register type.
// This value is determined at compile time by the compiler's ABI.
// For FFI, this is a crucial value for ensuring proper memory layout.
#if defined(__GNUC__) || defined(__clang__)
    #define PLATFORM_MAX_ALIGN_FUNDAMENTAL_TYPES __BIGGEST_ALIGNMENT__
#elif defined(_MSC_VER)
    // MSVC's default alignment for fundamental types is typically 8 bytes,
    // but can be higher for __m128, __m256, etc.
    // For simplicity, we assume 8 or 16.
    #if defined(PLATFORM_SIMD_AVX512F)
        #define PLATFORM_MAX_ALIGN_FUNDAMENTAL_TYPES 64 // AVX-512 vectors
    #elif defined(PLATFORM_SIMD_AVX) || defined(PLATFORM_SIMD_AVX2)
        #define PLATFORM_MAX_ALIGN_FUNDAMENTAL_TYPES 32 // AVX vectors
    #elif defined(PLATFORM_SIMD_SSE) || defined(PLATFORM_SIMD_SSE2) || defined(PLATFORM_SIMD_SSE3) || \
          defined(PLATFORM_SIMD_SSSE3) || defined(PLATFORM_SIMD_SSE4_1) || defined(PLATFORM_SIMD_SSE4_2)
        #define PLATFORM_MAX_ALIGN_FUNDAMENTAL_TYPES 16 // SSE vectors
    #else
        #define PLATFORM_MAX_ALIGN_FUNDAMENTAL_TYPES 8 // Default for long double, long long
    #endif
#else
    // Fallback for other compilers. This is a common safe default.
    #warning "Could not reliably determine PLATFORM_MAX_ALIGN_FUNDAMENTAL_TYPES. Assuming 16 bytes."
    #define PLATFORM_MAX_ALIGN_FUNDAMENTAL_TYPES 16
#endif

// Note on struct/byte alignment:
// The actual alignment of a struct depends on the alignment requirements of its members
// and the compiler's packing rules (e.g., #pragma pack, __attribute__((packed))).
// For explicit alignment control in FFI, use `alignas` (C++11/C11) or compiler-specific
// attributes like `__attribute__((aligned(N)))` (GCC/Clang) or `__declspec(align(N))` (MSVC).


// ----------------------------------------------------------------------------
// Section 10: Language Features / Type Support
// Defines macros for specific language features or built-in type support.
// ----------------------------------------------------------------------------

// These macros are defined if the corresponding feature is supported.
// Check for their presence using #ifdef.

#if defined(__SIZEOF_INT128__)
    #define PLATFORM_HAS_INT128 1
#endif

// Boolean type support
#if defined(__cplusplus)
    #define PLATFORM_HAS_BOOL 1 // 'bool' is a built-in type in C++
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) && (!defined(__STDC_NO_ATOMICS__) || !__STDC_NO_ATOMICS__)
    // _Bool is standard in C99 and later. Exclude if atomics are not supported (which might affect _Bool)
    #define PLATFORM_HAS_BOOL 1
#else
    // Fallback for older C standards or very constrained environments
    #warning "Platform may not have standard _Bool/bool support."
#endif

// long double type support
// The presence of sizeof(long double) already implies support, but this flag
// explicitly states it for feature detection.
#if defined(__FLT_EVAL_METHOD__) && (__FLT_EVAL_METHOD__ == 2)
    // FLT_EVAL_METHOD == 2 implies long double has higher precision than double
    #define PLATFORM_HAS_LONG_DOUBLE 1
#elif (LDBL_MANT_DIG > DBL_MANT_DIG) || (LDBL_MAX_EXP > DBL_MAX_EXP)
    // Check if long double has greater range/precision than double
    #define PLATFORM_HAS_LONG_DOUBLE 1
#elif defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)
    // Most modern compilers support long double, even if not explicitly
    // indicated by standard macros. Assume support if a major compiler.
    #define PLATFORM_HAS_LONG_DOUBLE 1
#else
    #warning "Could not reliably determine long double support. Assuming no explicit support."
#endif
