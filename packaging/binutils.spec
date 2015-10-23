%define binutils_target %{_target_platform}
%define isnative 1
%define enable_shared 1
%define run_testsuite 0
%define accelerator_crossbuild 0
%define disable_nls 1

%ifarch x86_64
%define x64 x64
%endif

%define linaro_release 2015.01
%define linaro_sub_release -2
%define release_prefix %{linaro_release}

Summary: A GNU collection of binary utilities
Name: binutils
Version: 2.25.0
Release: %{linaro_release}
License: GPLv3+
Group: Development/Tools
URL: http://www.linaro.org/
Source: binutils-linaro-%{version}-%{linaro_release}%{?linaro_sub_release}.tar.xz
Source2: binutils-2.19.50.0.1-output-format.sed
Source100: baselibs.conf
Source200: precheckin.sh
Source201: README.PACKAGER

%if "%{name}" != "binutils"
%if "%{name}" != "cross-mipsel-binutils"
%define binutils_target %(echo %{name} | sed -e "s/cross-\\(.*\\)-binutils/\\1/")-tizen-linux-gnueabi
%else
%define binutils_target %(echo %{name} | sed -e "s/cross-\\(.*\\)-binutils/\\1/")-tizen-linux-gnu
%endif
%define _prefix /opt/cross
%define enable_shared 0
%define isnative 0
%define run_testsuite 0
%define cross %{binutils_target}-
# single target atm.
ExclusiveArch: %ix86 x86_64
# special handling for Tizen ARM build acceleration
%if "%(echo %{name} | sed -e "s/cross-.*-binutils-\\(.*\\)-.*/\\1/")" == "accel"
%define binutils_target %(echo %{name} | sed -e "s/cross-\\(.*\\)-binutils-accel-.*/\\1/")-tizen-linux-gnueabi
%define _prefix /usr
%define cross ""
%define accelerator_crossbuild 1
AutoReqProv: 0
%define _build_name_fmt    %%{ARCH}/%%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.dontuse.rpm
%endif
%endif

BuildRequires: texinfo >= 4.0, gettext, flex, bison, zlib-devel
# Required for: ld-bootstrap/bootstrap.exp bootstrap with --static
# It should not be required for: ld-elf/elf.exp static {preinit,init,fini} array
%if %{run_testsuite}
BuildRequires: dejagnu, zlib-static, glibc-static, sharutils
%endif
BuildRequires: elfutils-libelf-devel
Conflicts: gcc-c++ < 4.0.0
Requires(post): /sbin/install-info
Requires(preun): /sbin/install-info

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
Requires(post): /sbin/install-info
Requires(preun): /sbin/install-info
Requires: zlib-devel

%description devel
This package contains BFD and opcodes static libraries and associated
header files.  Only *.a libraries are included, because BFD doesn't
have a stable ABI.  Developers starting new projects are strongly encouraged
to consider using libelf instead of BFD.

%prep
%setup -q -n binutils-linaro-%{version}-%{linaro_release}%{?linaro_sub_release}

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
export CFLAGS="$CFLAGS -Wl,-rpath,/emul/ia32-linux/usr/%{_lib}:/emul/ia32-linux/%{_lib}:/usr/%{_lib}:/%{_lib}:/usr/lib:/lib"
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
  --disable-werror \
  --enable-lto \
  --enable-gold=yes \
  --enable-plugins \
%if %{disable_nls}
  --disable-nls \
%endif
  --disable-gdb --disable-libdecnumber --disable-readline --disable-sim \
  --with-bugurl=http://bugs.tizen.org/
make %{_smp_mflags} tooldir=%{_prefix} all
make %{_smp_mflags} tooldir=%{_prefix} info

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
make prefix=%{buildroot}%{_prefix} infodir=%{buildroot}%{_infodir} install-info

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
# For cross-binutils we drop the documentation.
rm -rf %{buildroot}%{_infodir}
# We keep these as one can have native + cross binutils of different versions.
#rm -rf %{buildroot}%{_prefix}/share/locale
#rm -rf %{buildroot}%{_mandir}
rm -rf %{buildroot}%{_prefix}/%{_lib}/libiberty.a
%endif # !%{isnative}

# This one comes from gcc
rm -f %{buildroot}%{_infodir}/dir
rm -rf %{buildroot}%{_prefix}/%{binutils_target}

%if !%{disable_nls}
%find_lang %{?cross}binutils
%find_lang %{?cross}opcodes
%find_lang %{?cross}bfd
%find_lang %{?cross}gas
%find_lang %{?cross}ld
%find_lang %{?cross}gprof
cat %{?cross}opcodes.lang >> %{?cross}binutils.lang
cat %{?cross}bfd.lang >> %{?cross}binutils.lang
cat %{?cross}gas.lang >> %{?cross}binutils.lang
cat %{?cross}ld.lang >> %{?cross}binutils.lang
cat %{?cross}gprof.lang >> %{?cross}binutils.lang
%endif

%if %{accelerator_crossbuild}
# Fixed x86 dependencies
sed "s/@X86@/%{!?x64:x86}%{?x64}/g" -i %{_sourcedir}/baselibs.conf
%endif

%clean
rm -rf %{buildroot}

%if %{isnative}
%post
/sbin/ldconfig
%install_info --info-dir=%{_infodir} %{_infodir}/as.info
%install_info --info-dir=%{_infodir} %{_infodir}/binutils.info
%install_info --info-dir=%{_infodir} %{_infodir}/gprof.info
%install_info --info-dir=%{_infodir} %{_infodir}/ld.info
%install_info --info-dir=%{_infodir} %{_infodir}/standards.info
%install_info --info-dir=%{_infodir} %{_infodir}/configure.info
exit 0

%preun
if [ $1 = 0 ] ;then
  %install_info --delete --info-dir=%{_infodir} %{_infodir}/as.info
  %install_info --delete --info-dir=%{_infodir} %{_infodir}/binutils.info
  %install_info --delete --info-dir=%{_infodir} %{_infodir}/gprof.info
  %install_info --delete --info-dir=%{_infodir} %{_infodir}/ld.info
  %install_info --delete --info-dir=%{_infodir} %{_infodir}/standards.info
  %install_info --delete --info-dir=%{_infodir} %{_infodir}/configure.info
fi
exit 0

%postun -p /sbin/ldconfig

%post devel
%install_info --info-dir=%{_infodir} %{_infodir}/bfd.info

%preun devel
if [ $1 = 0 ] ;then
  %install_info --delete --info-dir=%{_infodir} %{_infodir}/bfd.info
fi
%endif # %{isnative}

%if %{disable_nls}
%files
%else
%files -f %{?cross}binutils.lang
%endif
%defattr(-,root,root,-)
%doc README
%{_prefix}/bin/*
%{_mandir}/man1/*
%if %{enable_shared}
%{_prefix}/%{_lib}/lib*.so
%exclude %{_prefix}/%{_lib}/libbfd.so
%exclude %{_prefix}/%{_lib}/libopcodes.so
%endif
%if %{isnative}
%{_infodir}/[^b]*info*
%{_infodir}/binutils*info*

%files devel
%defattr(-,root,root,-)
%{_prefix}/include/*
%{_prefix}/%{_lib}/libbfd.so
%{_prefix}/%{_lib}/libopcodes.so
%{_prefix}/%{_lib}/lib*.a
%{_infodir}/bfd*info*
%endif # %{isnative}

