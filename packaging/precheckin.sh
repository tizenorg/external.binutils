#!/bin/bash

NAME=binutils
SPECNAME=${NAME}.spec
ARCHES="armv7l"
TOBASELIBS=""
TOBASELIBS_ARCH=""


# baselibs.conf - part 1
mv -f baselibs.conf baselibs.conf.old  2>&1 > /dev/null || true
echo -n "arch i586 targets " > baselibs.conf


for i in ${ARCHES} ; do
# cross spec files
    cat ./${SPECNAME} | sed -e "s#Name: .*#Name: cross-${i}-${NAME}#" > ./cross-${i}-${NAME}.spec
    cat ./${SPECNAME} | sed -e "s#Name: .*#Name: cross-${i}-${NAME}-accel-%{!?x64:x86}%{?x64}#" > ./cross-${i}-${NAME}-accel.spec
# baselibs.conf - part 2
    test ! x"$i" = x"" && echo -n "${i}:${i} " >> baselibs.conf
done

echo "" >> baselibs.conf
cat baselibs.conf | sed -e "s/i586/x86_64/" >> baselibs.conf

# baselibs.conf - part 3
echo "" >> baselibs.conf
for i in ${ARCHES} ; do
echo "" >> baselibs.conf
echo "cross-${i}-${NAME}-accel-@X86@
  targettype x86 block!
  targettype 32bit block!" >> baselibs.conf
for j in ${ARCHES//${i}} ; do
  echo "  targettype $j block!" >> baselibs.conf
done
cat >> baselibs.conf << EOF

  targettype ${i} autoreqprov off
  targettype ${i} provides "cross-arm-binutils-accel"
  targettype ${i} provides "cross-${i}-binutils-accel-${i}"
  targettype ${i} requires "cross-arm-gcc-accel"
  targettype ${i} requires "glibc-@X86@-arm"
  targettype ${i} requires "zlib-@X86@-arm"
  targettype ${i} requires "binutils"
  targettype ${i} prefix /emul/ia32-linux
  targettype ${i} extension -arm
  targettype ${i} +/
  targettype ${i} -/usr/share/man
  targettype ${i} -/usr/share/doc
  targettype ${i} requires "tizen-accelerator"

  targettype ${i} post "#set -x"
  targettype ${i} post " export GCCVER=\$(LANG=C gcc -dumpversion) "
  targettype ${i} post " for bin in addr2line ar as c++filt elfedit gprof ld ld.bfd ld.gold nm objcopy objdump ranlib readelf size strings strip ; do"
  targettype ${i} post "   binary="/usr/bin/\${bin}" "
  targettype ${i} post "   if test -L \${binary} -a -e \${binary}.orig-arm ; then"
  targettype ${i} post "     echo "\${binary} not installed or \${binary}.orig-arm already present !" "
  targettype ${i} post "   else "
  targettype ${i} post "     mv \${binary} \${binary}.orig-arm && ln -s <prefix>\${binary} \${binary}"
  targettype ${i} post "     ln -s \${binary} /usr/lib/gcc/${i}-tizen-linux-gnueabi/\${GCCVER}/\${bin}"
  targettype ${i} post "   fi "
  targettype ${i} post " done "
  targettype ${i} post " ln -sf /usr/bin/ld /usr/lib/gcc/${i}-tizen-linux-gnueabi/\${GCCVER}/ld"
  targettype ${i} post " ln -sf /usr/bin/ld.bfd /usr/lib/gcc/${i}-tizen-linux-gnueabi/\${GCCVER}/ld.bfd"
  targettype ${i} post " ln -sf /usr/bin/ld.gold /usr/lib/gcc/${i}-tizen-linux-gnueabi/\${GCCVER}/ld.gold"

  targettype ${i} preun " set -x"
  targettype ${i} preun " export GCCVER=\$(LANG=C gcc -dumpversion) "
  targettype ${i} preun " for bin in addr2line ar as c++filt elfedit gprof ld ld.bfd ld.gold nm objcopy objdump ranlib readelf size strings strip ; do"
  targettype ${i} preun "   binary="/usr/bin/\${bin}" "
  targettype ${i} preun "   if test -e \${binary}.orig-arm ; then"
  targettype ${i} preun "     rm \${binary} && mv \${binary}.orig-arm \${binary}"
  targettype ${i} preun "     rm /usr/lib/gcc/${i}-tizen-linux-gnueabi/\${GCCVER}/\${bin}"
  targettype ${i} preun "   else "
  targettype ${i} preun "     echo "\${binary}.orig-arm not present !" "
  targettype ${i} preun "   fi "
  targettype ${i} preun " done "
  targettype ${i} preun " rm -f /usr/lib/gcc/${i}-tizen-linux-gnueabi/\${GCCVER}/ld"
  targettype ${i} preun " rm -f /usr/lib/gcc/${i}-tizen-linux-gnueabi/\${GCCVER}/ld.bfd"
  targettype ${i} preun " rm -f /usr/lib/gcc/${i}-tizen-linux-gnueabi/\${GCCVER}/ld.gold"

EOF


done


exit 0
