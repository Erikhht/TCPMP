#!/bin/sh

MODE=palmos
PREFIX=/usr/arm-tcpmp-palmos
TARGET=arm-tcpmp-palmos
BINUTILS="binutils-2.14"
GCC="gcc-3.4.3"
PRCTOOLS="prc-tools-2.3"

PATH=$PATH:$PREFIX/bin
SRCDIR="`pwd`"
TMPDIR="/tmp/tcpmp"
PATCH="patch -p1"
MAKE="make"
WGET="wget"

#-------------
# 1. download
#-------------

if test ! -f "$BINUTILS.tar.gz" ; then
  $WGET ftp://ftp.gnu.org/pub/gnu/binutils/$BINUTILS.tar.gz || { echo "error downloading binutils"; exit; }
fi

if test ! -f "$GCC.tar.bz2" ; then
  $WGET ftp://ftp.gnu.org/pub/gnu/gcc/$GCC/$GCC.tar.bz2 || { echo "error downloading gcc"; exit; }
fi

if test ! -f "$PRCTOOLS.tar.gz" ; then
  $WGET http://puzzle.dl.sourceforge.net/sourceforge/prc-tools/$PRCTOOLS.tar.gz || { echo "error downloading prc-tools"; exit; }
fi

#------------------------
# 2. unpack and patching
#------------------------

mkdir -p "$TMPDIR"; cd "$TMPDIR"

rm -Rf $BINUTILS
gzip -cd "$SRCDIR/$BINUTILS.tar.gz" | tar xvf -
cd $BINUTILS
cat "$SRCDIR/$BINUTILS.$MODE.diff" | $PATCH || { echo "error patching binutils"; exit; }
cd ..

rm -Rf $GCC
bzip2 -cd "$SRCDIR/$GCC.tar.bz2" | tar xvf -
cd $GCC
cat "$SRCDIR/$GCC.$MODE.diff" | $PATCH || { echo "error patching gcc"; exit; }
cd ..

rm -Rf $PRCTOOLS
gzip -cd "$SRCDIR/$PRCTOOLS.tar.gz" | tar xvf -
cp "$SRCDIR/libc-palmos/Makefile" $PRCTOOLS/libc

#------------------------
# 3. making and install
#------------------------

mkdir -p $PREFIX/$TARGET
cp -r /usr/arm-palmos $PREFIX || { echo "error coping arm-palmos"; exit; }
mv $PREFIX/arm-palmos $PREFIX/$TARGET
mkdir -p $PREFIX/share/prc-tools
cp -r /usr/share/prc-tools/include $PREFIX/share/prc-tools/ || { echo "error coping prc-tools include directory"; exit; }

mkdir $BINUTILS/build-$MODE
mkdir $GCC/build-$MODE

cd $BINUTILS/build-$MODE
../configure --target=$TARGET --prefix=$PREFIX --disable-nls || { echo "error config binutils"; exit; }
$MAKE clean
$MAKE all || { echo "error making binuitls"; exit; }
$MAKE install || { echo "error installing binuitls"; exit; }
cd ../..

cd $GCC/build-$MODE
../configure --target=$TARGET --prefix=$PREFIX --disable-nls \
    --enable-languages=c,c++ --without-headers --with-newlib || { echo "error config gcc"; exit; }
$MAKE clean
$MAKE all-gcc || { echo "error making gcc"; exit; }
$MAKE install-gcc || { echo "error installing gcc"; exit; }
cd ../..

if test -f "/usr/lib/gcc-lib/arm-palmos/specs" ; then
  #linux
  cp /usr/lib/gcc-lib/arm-palmos/specs $PREFIX/lib/gcc/$TARGET  || { echo "error coping specs"; exit; }
else
  #cygwin
  cp /lib/gcc-lib/arm-palmos/specs $PREFIX/lib/gcc/$TARGET || { echo "error coping specs"; exit; }
fi

cd $PRCTOOLS/libc
$MAKE clean
$MAKE all || { echo "error making prc-tools libc"; exit; }
$MAKE install || { echo "error installing prc-tools libc"; exit; }
cd ../..

#------------------------
# 4. clean up
#------------------------

rm -Rf $BINUTILS
rm -Rf $GCC
rm -Rf $PRCTOOLS
