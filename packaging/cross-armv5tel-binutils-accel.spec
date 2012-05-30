%define binutils_target %{_target_platform}
%define isnative 1
%define enable_shared 1
%define run_testsuite 0
%define accelerator_crossbuild 0

Summary: A GNU collection of binary utilities
Name: cross-armv5tel-binutils-accel
Version: 2.21.51.20110421
Release: 5
License: GPLv3+
Group: Development/Tools
URL: http://sources.redhat.com/binutils
Source: ftp://ftp.kernel.org/pub/linux/devel/binutils/binutils-%{version}.tar.gz
Source2: binutils-2.19.50.0.1-output-format.sed
Source100: baselibs.conf
Source200: precheckin.sh
Source201: README.PACKAGER
Source1001: packaging/cross-armv5tel-binutils-accel.manifest 
Patch1: 001_ld_makefile_patch.patch
Patch2: 006_better_file_error.patch
Patch3: 012_check_ldrunpath_length.patch
Patch4: 013_bash_in_ld_testsuite.patch
Patch5: 127_x86_64_i386_biarch.patch
Patch6: 128_build_id.patch
Patch7: 129_ld_mulitarch_dirs.patch
Patch8: 130_gold_disable_testsuite_build.patch
Patch9: 131_ld_bootstrap_testsuite.patch
Patch10: 134_gold_no_spu.patch
Patch11: 135_bfd_version.patch
Patch12: 140_pr10340.patch
Patch13: 156_pr10144.patch
Patch14: 157_ar_scripts_with_tilde.patch
Patch15: 158_ld_system_root.patch
Patch16: 159_gas-i8775.diff
Patch17: 160_gas_pr12698.diff
Patch18: 161_ar_delete_members.diff
Patch19: 162_pr12730.diff
Patch20: 162_ld_cortex_a8_erratum.diff
Patch21: 163_pr12726.diff
Patch22: 200_pr12715.diff
Patch23: 201_pr12778.diff
Patch24: Fix-gold-libopcode.patch
Patch25: Disable-info.patch

%if "%{name}" != "binutils"
%define binutils_target %(echo %{name} | sed -e "s/cross-\\(.*\\)-binutils/\\1/")-tizen-linux-gnueabi
%define _prefix /opt/cross
%define enable_shared 0
%define isnative 0
%define run_testsuite 0
%define cross %{binutils_target}-
# single target atm.
ExclusiveArch: %ix86
# special handling for ARM build acceleration
%if "%(echo %{name} | sed -e "s/cross-.*-binutils-\\(.*\\)/\\1/")" == "accel"
%define binutils_target %(echo %{name} | sed -e "s/cross-\\(.*\\)-binutils-accel/\\1/")-tizen-linux-gnueabi
%define _prefix /usr
%define cross ""
%define accelerator_crossbuild 1
AutoReqProv: 0
%define _build_name_fmt    %%{ARCH}/%%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.dontuse.rpm
%endif
%endif

#BuildRequires: gettext
BuildRequires: flex
BuildRequires: bison
BuildRequires: zlib-devel
BuildRequires: elfutils-libelf-devel
# Required for: ld-bootstrap/bootstrap.exp bootstrap with --static
# It should not be required for: ld-elf/elf.exp static {preinit,init,fini} array
%if %{run_testsuite}
BuildRequires: dejagnu, zlib-static, glibc-static, sharutils
%endif
Conflicts: gcc-c++ < 4.0.0

# On ARM EABI systems, we do want -gnueabi to be part of the
# target triple.
%ifnarch %{arm}
%define _gnu %{nil}
%endif

%description
Binutils is a collection of binary utilities, including ar (for
creating, modifying and extracting from archives), as (a family of GNU
assemblers), gprof (for displaying call graph profile data), ld (the
GNU linker), nm (for listing symbols from object files), objcopy (for
copying and translating object files), objdump (for displaying
information from object files), ranlib (for generating an index for
the contents of an archive), readelf (for displaying detailed
information about binary files), size (for listing the section sizes
of an object or archive file), strings (for listing printable strings
from files), strip (for discarding symbols), and addr2line (for
converting addresses to file and line).

%package devel
Summary: BFD and opcodes static libraries and header files
Group: System/Libraries
Conflicts: binutils < 2.17.50.0.3-4
Requires: zlib-devel

%description devel
This package contains BFD and opcodes static libraries and associated
header files.  Only *.a libraries are included, because BFD doesn't
have a stable ABI.  Developers starting new projects are strongly encouraged
to consider using libelf instead of BFD.

%prep
%setup -q -n binutils-%{version}
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1
%patch6 -p1
%patch7 -p1
%patch8 -p1
%patch9 -p1
%patch10 -p1
%patch11 -p1
%patch12 -p1
%patch13 -p1
%patch14 -p1
%patch15 -p1
%patch16 -p1
%patch17 -p1
%patch18 -p1
%patch19 -p1
%patch20 -p1
%patch21 -p1
%patch22 -p1
%patch23 -p1
%patch24 -p1
%patch25 -p1


# We cannot run autotools as there is an exact requirement of autoconf-2.59.

# On ppc64 we might use 64KiB pages
sed -i -e '/#define.*ELF_COMMONPAGESIZE/s/0x1000$/0x10000/' bfd/elf*ppc.c
# LTP sucks
perl -pi -e 's/i\[3-7\]86/i[34567]86/g' */conf*
sed -i -e 's/%''{release}/%{release}/g' bfd/Makefile{.am,.in}
sed -i -e '/^libopcodes_la_\(DEPENDENCIES\|LIBADD\)/s,$, ../bfd/libbfd.la,' opcodes/Makefile.{am,in}
# Build libbfd.so and libopcodes.so with -Bsymbolic-functions if possible.
if gcc %{optflags} -v --help 2>&1 | grep -q -- -Bsymbolic-functions; then
sed -i -e 's/^libbfd_la_LDFLAGS = /&-Wl,-Bsymbolic-functions /' bfd/Makefile.{am,in}
sed -i -e 's/^libopcodes_la_LDFLAGS = /&-Wl,-Bsymbolic-functions /' opcodes/Makefile.{am,in}
fi
# $PACKAGE is used for the gettext catalog name.
sed -i -e 's/^ PACKAGE=/ PACKAGE=%{?cross}/' */configure
# Undo the name change to run the testsuite.
for tool in binutils gas ld
do
  sed -i -e "2aDEJATOOL = $tool" $tool/Makefile.am
  sed -i -e "s/^DEJATOOL = .*/DEJATOOL = $tool/" $tool/Makefile.in
done
touch */configure

%build
cp %{SOURCE1001} .
echo target is %{binutils_target}
export CFLAGS="$RPM_OPT_FLAGS"
CARGS=

case %{binutils_target} in i?86*)
  CARGS="$CARGS --enable-64-bit-bfd"
  ;;
esac

%if 0%{?_with_debug:1}
CFLAGS="$CFLAGS -O0 -ggdb2"
%define enable_shared 0
%endif

# We could optimize the cross builds size by --enable-shared but the produced
# binaries may be less convenient in the embedded environment.
%if %{accelerator_crossbuild}
export CFLAGS="$CFLAGS -Wl,-rpath,/emul/ia32-linux/usr/lib:/emul/ia32-linux/lib:/usr/lib:/lib"
%endif
%configure \
  --build=%{_target_platform} --host=%{_target_platform} \
  --target=%{binutils_target} \
%if !%{isnative} 
%if !%{accelerator_crossbuild}
  --enable-targets=%{_host} \
  --with-sysroot=%{_prefix}/%{binutils_target}/sys-root \
  --program-prefix=%{cross} \
%else
  --with-sysroot=/ \
  --program-prefix="" \
%endif
%endif
%if %{enable_shared}
  --enable-shared \
%else
  --disable-shared \
%endif
  $CARGS \
  --enable-plugins \
  --disable-werror \
  --enable-ld=default \
  --disable-info \
  --enable-gold

make %{_smp_mflags} tooldir=%{_prefix} all

# Do not use %%check as it is run after %%install where libbfd.so is rebuild
# with -fvisibility=hidden no longer being usable in its shared form.
%if !%{run_testsuite}
echo ====================TESTSUITE DISABLED=========================
%else
make -k check < /dev/null || :
echo ====================TESTING=========================
cat {gas/testsuite/gas,ld/ld,binutils/binutils}.sum
echo ====================TESTING END=====================
for file in {gas/testsuite/gas,ld/ld,binutils/binutils}.{sum,log}
do
  ln $file binutils-%{_target_platform}-$(basename $file) || :
done
tar cjf binutils-%{_target_platform}.tar.bz2 binutils-%{_target_platform}-*.{sum,log}
uuencode binutils-%{_target_platform}.tar.bz2 binutils-%{_target_platform}.tar.bz2
rm -f binutils-%{_target_platform}.tar.bz2 binutils-%{_target_platform}-*.{sum,log}
%endif

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
%if %{isnative}

# Rebuild libiberty.a with -fPIC.
# Future: Remove it together with its header file, projects should bundle it.
make -C libiberty clean
make CFLAGS="-g -fPIC $RPM_OPT_FLAGS" -C libiberty

# Rebuild libbfd.a with -fPIC.
# Without the hidden visibility the 3rd party shared libraries would export
# the bfd non-stable ABI.
make -C bfd clean
make CFLAGS="-g -fPIC $RPM_OPT_FLAGS -fvisibility=hidden" -C bfd

install -m 644 bfd/libbfd.a %{buildroot}%{_prefix}/%{_lib}
install -m 644 libiberty/libiberty.a %{buildroot}%{_prefix}/%{_lib}
install -m 644 include/libiberty.h %{buildroot}%{_prefix}/include
# Remove Windows/Novell only man pages
rm -f %{buildroot}%{_mandir}/man1/{dlltool,nlmconv,windres}*

%if %{enable_shared}
chmod +x %{buildroot}%{_prefix}/%{_lib}/lib*.so*
%endif

# Prevent programs to link against libbfd and libopcodes dynamically,
# they are changing far too often
rm -f %{buildroot}%{_prefix}/%{_lib}/lib{bfd,opcodes}.so

# Remove libtool files, which reference the .so libs
rm -f %{buildroot}%{_prefix}/%{_lib}/lib{bfd,opcodes}.la

%if "%{__isa_bits}" == "64"
# Sanity check --enable-64-bit-bfd really works.
grep '^#define BFD_ARCH_SIZE 64$' %{buildroot}%{_prefix}/include/bfd.h
%endif
# Fix multilib conflicts of generated values by __WORDSIZE-based expressions.
%ifarch %{ix86} x86_64 
sed -i -e '/^#include "ansidecl.h"/{p;s~^.*$~#include <bits/wordsize.h>~;}' \
    -e 's/^#define BFD_DEFAULT_TARGET_SIZE \(32\|64\) *$/#define BFD_DEFAULT_TARGET_SIZE __WORDSIZE/' \
    -e 's/^#define BFD_HOST_64BIT_LONG [01] *$/#define BFD_HOST_64BIT_LONG (__WORDSIZE == 64)/' \
    -e 's/^#define BFD_HOST_64_BIT \(long \)\?long *$/#if __WORDSIZE == 32\
#define BFD_HOST_64_BIT long long\
#else\
#define BFD_HOST_64_BIT long\
#endif/' \
    -e 's/^#define BFD_HOST_U_64_BIT unsigned \(long \)\?long *$/#define BFD_HOST_U_64_BIT unsigned BFD_HOST_64_BIT/' \
    %{buildroot}%{_prefix}/include/bfd.h
%endif
touch -r bfd/bfd-in2.h %{buildroot}%{_prefix}/include/bfd.h

# Generate .so linker scripts for dependencies; imported from glibc/Makerules:

# This fragment of linker script gives the OUTPUT_FORMAT statement
# for the configuration we are building.
OUTPUT_FORMAT="\
/* Ensure this .so library will not be used by a link for a different format
   on a multi-architecture system.  */
$(gcc $CFLAGS $LDFLAGS -shared -x c /dev/null -o /dev/null -Wl,--verbose -v 2>&1 | sed -n -f "%{SOURCE2}")"

tee %{buildroot}%{_prefix}/%{_lib}/libbfd.so <<EOH
/* GNU ld script */

$OUTPUT_FORMAT

/* The libz dependency is unexpected by legacy build scripts.  */
INPUT ( %{_libdir}/libbfd.a -liberty -lz )
EOH

tee %{buildroot}%{_prefix}/%{_lib}/libopcodes.so <<EOH
/* GNU ld script */

$OUTPUT_FORMAT

INPUT ( %{_libdir}/libopcodes.a -lbfd )
EOH

%else # !%{isnative}
# We keep these as one can have native + cross binutils of different versions.
#rm -rf %{buildroot}%{_prefix}/share/locale
#rm -rf %{buildroot}%{_mandir}
rm -rf %{buildroot}%{_prefix}/%{_lib}/libiberty.a
%endif # !%{isnative}

# This one comes from gcc
rm -f %{buildroot}%{_infodir}/dir
rm -rf %{buildroot}%{_prefix}/%{binutils_target}

# As gas/README points out (search for --enable-targets),
# multi-arch gas is not ready yet.
rm -rf %{buildroot}%{_libdir}/ldscripts


%postun -p /sbin/ldconfig

%files 
%manifest cross-armv5tel-binutils-accel.manifest
%defattr(-,root,root,-)
%{_prefix}/bin/*
%{_mandir}/man1/*
%if %{enable_shared}
%{_prefix}/%{_lib}/lib*.so
%exclude %{_prefix}/%{_lib}/libbfd.so
%exclude %{_prefix}/%{_lib}/libopcodes.so
%endif
%if %{isnative}

%files devel
%manifest cross-armv5tel-binutils-accel.manifest
%defattr(-,root,root,-)
%{_prefix}/include/*
%{_prefix}/%{_lib}/libbfd.so
%{_prefix}/%{_lib}/libopcodes.so
%{_prefix}/%{_lib}/lib*.a
%endif # %{isnative}

