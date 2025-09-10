#include "../Affix.h"

#define _eval(x) #x  // extra round of macroexpansion
#define _is_defined(x, y) strcmp(x, y)
#define is_defined(x) _is_defined(#x, _eval(x))

IV padding_needed_for(IV offset, IV alignment) {
    //~ std::cerr << "padding_needed_for( " << offset << ", " << alignment << " )" << std::endl;
    if (offset == 0)
        return alignment;
    if (alignment == 0)
        return 0;
    IV misalignment = offset % alignment;
    if (misalignment != 0)
        return alignment - misalignment;  // round to the next multiple of alignment
    return 0;                             // already a multiple of alignment
}

XS_INTERNAL(Affix_Platform_padding) {
    dXSARGS;
    PERL_UNUSED_VAR(items);
    XSRETURN_IV(padding_needed_for(SvIV(ST(0)), SvIV(ST(1))));
}

void boot_Affix_Platform(pTHX_ CV * cv) {
    PERL_UNUSED_VAR(cv);

    (void)newXSproto_portable("Affix::Platform::padding_needed_for", Affix_Platform_padding, __FILE__, "$$");

    // dyncall/dyncall_version.h
    register_constant("Affix::Platform",
                      "DC_Version",
                      Perl_newSVpvf(aTHX_ "%d.%d-%7s",
                                    (DYNCALL_VERSION >> 12),
                                    (DYNCALL_VERSION >> 8) & 0xf,
                                    (((DYNCALL_VERSION & 0xf) == 0xf) ? "release" : "current")));
    register_constant("Affix::Platform", "DC_Major", newSViv(DYNCALL_VERSION >> 12));
    register_constant("Affix::Platform", "DC_Minor", newSViv((DYNCALL_VERSION >> 8) & 0xf));
    register_constant("Affix::Platform", "DC_Patch", newSViv((DYNCALL_VERSION >> 4) & 0xf));
    register_constant(
        "Affix::Platform", "DC_Stage", newSVpv((((DYNCALL_VERSION & 0xf) == 0xf) ? "release" : "current"), 7));
    register_constant("Affix::Platform", "DC_RawVersion", newSViv(DYNCALL_VERSION));

    // https://dyncall.org/pub/dyncall/dyncall/file/tip/dyncall/dyncall_macros.h
    const char * os =
#if defined DC__OS_Win64
        "Win64"
#elif defined DC__OS_Win32
        "Win32"
#elif defined DC__OS_MacOSX
        "macOS"
#elif defined DC__OS_IPhone
        "iOS"
#elif defined DC__OS_Linux
        "Linux"
#elif defined DC__OS_FreeBSD
        "FreeBSD"
#elif defined DC__OS_OpenBSD
        "OpenBSD"
#elif defined DC__OS_NetBSD
        "NetBSD"
#elif defined DC__OS_DragonFlyBSD
        "DragonFly BSD"
#elif defined DC__OS_NDS
        "Nintendo DS"
#elif defined DC__OS_PSP
        "PlayStation Portable"
#elif defined DC__OS_BeOS
        "Haiku"
#elif defined DC__OS_Plan9
        "Plan9"
#elif defined DC__OS_VMS
        "VMS"
#elif defined DC__OS_Minix
        "Minix"
#else
        "Unknown"
#endif
        ;

    const char * compiler =
#if defined DC__C_Intel
        "Intel"
#elif defined DC__C_MSVC
        "MSVC"
#elif defined DC__C_CLANG
        "Clang"
#elif defined DC__C_GNU
        "GNU"
#elif defined DC__C_WATCOM
        "Watcom"
#elif defined DC__C_PCC
        "ppc"
#elif defined DC__C_SUNPRO
        "Oracle"
#else
        "Unknown"
#endif
        ;
    const char * architecture =
#if (defined(__arm64__) || defined(__arm64e__) || defined(__aarch64__)) && defined(DC__OS_MacOSX)
        "Apple Silicon"
#elif defined DC__Arch_AMD64
        "x86_64"
#elif defined DC__Arch_Intel_x86
        "x86"
#elif defined DC__Arch_Itanium
        "Itanium"
#elif defined DC__Arch_PPC64
        "PPC64"
#elif defined DC__Arch_PPC64
        "PPC32"
#elif defined DC__Arch_MIPS64
        "MIPS64"
#elif defined DC__Arch_MIPS
        "MIPS"
#elif defined DC__Arch_ARM
        "ARM"
#elif defined DC__Arch_ARM64
        "ARM64"
#elif defined DC__Arch_SuperH
        "SuperH"  // https://en.wikipedia.org/wiki/SuperH
#elif defined DC__Arch_Sparc64
        "SPARC64"
#elif defined DC__Arch_Sparc
        "SPARC"
#else
        "Unknown"
#endif
        ;

    const char * obj =
#ifdef DC__Obj_PE
        "PE"
#elif defined DC__Obj_Mach
        "Mach-O"
#elif defined DC__Obj_ELF64
        "64-bit ELF"
#elif defined DC__Obj_ELF32
        "32-bit ELF"
#elif defined DC__Obj_ELF
        "ELF"
#else
        "Unknown"
#endif
        ;

    // Basics
    register_constant("Affix::Platform", "OS", newSVpv(os, 0));
    register_constant("Affix::Platform", "Cygwin", boolSV(is_defined(DC__OS_Cygwin)));
    register_constant("Affix::Platform", "MinGW", boolSV(is_defined(DC__OS_MinGW)));
    register_constant("Affix::Platform", "Compiler", newSVpv(compiler, 0));
    register_constant("Affix::Platform", "Architecture", newSVpv(architecture, 0));

    // Architecture types - undocumented
    register_constant("Affix::Platform",
                      "ARCH_Apple_Silicon",
                      boolSV((is_defined(__arm64__) || is_defined(__arm64e__) || is_defined(__aarch64__)) &&
                             is_defined(DC__OS_MacOSX)));
    register_constant("Affix::Platform", "ARCH_x86_64", boolSV(is_defined(DC__Arch_AMD64)));
    register_constant("Affix::Platform", "ARCH_x86", boolSV(is_defined(DC__Arch_Intel_x86)));
    register_constant("Affix::Platform", "ARCH_Itanium", boolSV(is_defined(DC__Arch_Itanium)));
    register_constant("Affix::Platform", "ARCH_PPC64", boolSV(is_defined(DC__Arch_PPC64)));
    register_constant("Affix::Platform", "ARCH_PPC32", boolSV(is_defined(DC__Arch_PPC32)));
    register_constant("Affix::Platform", "ARCH_MIPS64", boolSV(is_defined(DC__Arch_MIPS64)));
    register_constant("Affix::Platform", "ARCH_MIPS", boolSV(is_defined(DC__Arch_MIPS)));
    register_constant("Affix::Platform", "ARCH_ARM", boolSV(is_defined(DC__Arch_ARM)));
    register_constant("Affix::Platform", "ARCH_ARM64", boolSV(is_defined(DC__Arch_ARM64)));
    register_constant("Affix::Platform", "ARCH_SuperH", boolSV(is_defined(DC__Arch_SuperH)));
    register_constant("Affix::Platform", "ARCH_Sparc64", boolSV(is_defined(DC__Arch_Sparc64)));
    register_constant("Affix::Platform", "ARCH_Sparc", boolSV(is_defined(DC__Arch_Sparc)));

    // Architecture
    register_constant("Affix::Platform", "ARM_Thumb", boolSV(is_defined(DC__Arch_ARM_THUMB)));
    register_constant("Affix::Platform", "ARM_EABI", boolSV(is_defined(DC__ABI_ARM_EABI)));
    register_constant("Affix::Platform", "ARM_OABI", boolSV(is_defined(DC__ABI_ARM_OABI)));
    register_constant("Affix::Platform", "MIPS_O32", boolSV(is_defined(DC__ABI_MIPS_O32)));
    register_constant("Affix::Platform", "MIPS_N64", boolSV(is_defined(DC__ABI_MIPS_N64)));
    register_constant("Affix::Platform", "MIPS_N32", boolSV(is_defined(DC__ABI_MIPS_N32)));
    register_constant("Affix::Platform", "MIPS_EABI", boolSV(is_defined(DC__ABI_MIPS_EABI)));

    //
    register_constant(
        "Affix::Platform", "HardFloat", boolSV(is_defined(DC__ABI_ARM_HF) || is_defined(DC__ABI_HARDFLOAT)));
    register_constant("Affix::Platform", "BigEndian", boolSV(is_defined(DC__Endian_BIG)));

    // Features
    register_constant("Affix::Platform", "Syscall", boolSV(is_defined(DC__Feature_Syscall)));
    register_constant("Affix::Platform", "AggrByValue", boolSV(is_defined(DC__Feature_AggrByVal)));

    // OBJ types
    register_constant("Affix::Platform", "OBJ_PE", boolSV(is_defined(DC__Obj_PE)));
    register_constant("Affix::Platform", "OBJ_Mach", boolSV(is_defined(DC__Obj_Mach)));
    register_constant("Affix::Platform", "OBJ_ELF", boolSV(is_defined(DC__Obj_ELF)));
    register_constant("Affix::Platform", "OBJ_ELF64", boolSV(is_defined(DC__Obj_ELF64)));
    register_constant("Affix::Platform", "OBJ_ELF32", boolSV(is_defined(DC__Obj_ELF32)));
    register_constant("Affix::Platform", "OBJ", newSVpv(obj, 0));

    // sizeof
    export_constant("Affix::Platform", "SIZEOF_BOOL", "sizeof", SIZEOF_BOOL);
    export_constant("Affix::Platform", "SIZEOF_CHAR", "sizeof", SIZEOF_CHAR);
    export_constant("Affix::Platform", "SIZEOF_SCHAR", "sizeof", SIZEOF_SCHAR);
    export_constant("Affix::Platform", "SIZEOF_UCHAR", "sizeof", SIZEOF_UCHAR);
    export_constant("Affix::Platform", "SIZEOF_WCHAR", "sizeof", SIZEOF_WCHAR);
    export_constant("Affix::Platform", "SIZEOF_SHORT", "sizeof", SIZEOF_SHORT);
    export_constant("Affix::Platform", "SIZEOF_USHORT", "sizeof", SIZEOF_USHORT);
    export_constant("Affix::Platform", "SIZEOF_INT", "sizeof", SIZEOF_INT);
    export_constant("Affix::Platform", "SIZEOF_UINT", "sizeof", SIZEOF_UINT);
    export_constant("Affix::Platform", "SIZEOF_LONG", "sizeof", SIZEOF_LONG);
    export_constant("Affix::Platform", "SIZEOF_ULONG", "sizeof", SIZEOF_ULONG);
    export_constant("Affix::Platform", "SIZEOF_LONGLONG", "sizeof", SIZEOF_LONGLONG);
    export_constant("Affix::Platform", "SIZEOF_ULONGLONG", "sizeof", SIZEOF_ULONGLONG);
    export_constant("Affix::Platform", "SIZEOF_FLOAT", "sizeof", SIZEOF_FLOAT);
    export_constant("Affix::Platform", "SIZEOF_DOUBLE", "sizeof", SIZEOF_DOUBLE);
    export_constant("Affix::Platform", "SIZEOF_SIZE_T", "sizeof", SIZEOF_SIZE_T);
    export_constant("Affix::Platform", "SIZEOF_SSIZE_T", "sizeof", SIZEOF_SSIZE_T);
    export_constant("Affix::Platform", "SIZEOF_INTPTR_T", "sizeof", SIZEOF_INTPTR_T);

    // to calculate offsetof and padding inside structs
    export_constant("Affix::Platform", "BYTE_ALIGN", "all", AFFIX_ALIGNBYTES);  // platform
    export_constant("Affix::Platform", "ALIGNOF_BOOL", "all", ALIGNOF_BOOL);
    export_constant("Affix::Platform", "ALIGNOF_CHAR", "all", ALIGNOF_CHAR);
    export_constant("Affix::Platform", "ALIGNOF_UCHAR", "all", ALIGNOF_UCHAR);
    export_constant("Affix::Platform", "ALIGNOF_SCHAR", "all", ALIGNOF_SCHAR);
    export_constant("Affix::Platform", "ALIGNOF_WCHAR", "all", ALIGNOF_WCHAR);
    export_constant("Affix::Platform", "ALIGNOF_SHORT", "all", ALIGNOF_SHORT);
    export_constant("Affix::Platform", "ALIGNOF_USHORT", "all", ALIGNOF_USHORT);
    export_constant("Affix::Platform", "ALIGNOF_INT", "all", ALIGNOF_INT);
    export_constant("Affix::Platform", "ALIGNOF_UINT", "all", ALIGNOF_UINT);
    export_constant("Affix::Platform", "ALIGNOF_LONG", "all", ALIGNOF_LONG);
    export_constant("Affix::Platform", "ALIGNOF_ULONG", "all", ALIGNOF_ULONG);
    export_constant("Affix::Platform", "ALIGNOF_LONGLONG", "all", ALIGNOF_LONGLONG);
    export_constant("Affix::Platform", "ALIGNOF_ULONGLONG", "all", ALIGNOF_ULONGLONG);
    export_constant("Affix::Platform", "ALIGNOF_FLOAT", "all", ALIGNOF_FLOAT);
    export_constant("Affix::Platform", "ALIGNOF_DOUBLE", "all", ALIGNOF_DOUBLE);
    export_constant("Affix::Platform", "ALIGNOF_SIZE_T", "all", ALIGNOF_SIZE_T);
    export_constant("Affix::Platform", "ALIGNOF_SSIZE_T", "all", ALIGNOF_SSIZE_T);
    export_constant("Affix::Platform", "ALIGNOF_INTPTR_T", "all", ALIGNOF_INTPTR_T);

    // Undocumented
    register_constant("Affix::Platform", "Windows", boolSV(is_defined(DC__OS_Win64) || is_defined(DC__OS_Win32)));
    register_constant("Affix::Platform", "Win64", boolSV(is_defined(DC__OS_Win64)));
    register_constant("Affix::Platform", "Win32", boolSV(is_defined(DC__OS_Win32)));
    register_constant("Affix::Platform", "macOS", boolSV(is_defined(DC__OS_MacOSX)));
    register_constant("Affix::Platform", "iPhone", boolSV(is_defined(DC__OS_IPhone)));
    register_constant("Affix::Platform", "Linux", boolSV(is_defined(DC__OS_Linux)));
    register_constant("Affix::Platform", "FreeBSD", boolSV(is_defined(DC__OS_FreeBSD)));
    register_constant("Affix::Platform", "OpenBSD", boolSV(is_defined(DC__OS_OpenBSD)));
    register_constant("Affix::Platform", "NetBSD", boolSV(is_defined(DC__OS_NetBSD)));
    register_constant("Affix::Platform", "DragonFlyBSD", boolSV(is_defined(DC__OS_DragonFlyBSD)));
    register_constant("Affix::Platform", "NintendoDS", boolSV(is_defined(DC__OS_NDS)));
    register_constant("Affix::Platform", "SonyPSP", boolSV(is_defined(DC__OS_PSP)));
    register_constant("Affix::Platform", "BeOS", boolSV(is_defined(DC__OS_BeOS)));
    register_constant("Affix::Platform", "Plan9", boolSV(is_defined(DC__OS_Plan9)));
    register_constant("Affix::Platform", "VMS", boolSV(is_defined(DC__OS_VMS)));
    register_constant("Affix::Platform", "Minix", boolSV(is_defined(DC__OS_Minix)));
}
