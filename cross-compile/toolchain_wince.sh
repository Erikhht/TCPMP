#!/bin/sh

MODE=wince
PREFIX=/usr/arm-wince
TARGET=arm-wince-pe
BINUTILS="binutils-2.16"
GCC="gcc-3.4.3"

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

#------------------------
# 2. unpack and patching
#------------------------

mkdir -p "$TMPDIR"; cd "$TMPDIR"

rm -Rf $BINUTILS
gzip -cd "$SRCDIR/$BINUTILS.tar.gz" | tar xvf -

rm -Rf $GCC
bzip2 -cd "$SRCDIR/$GCC.tar.bz2" | tar xvf -
cd $GCC
cat "$SRCDIR/$GCC.$MODE.diff" | $PATCH || { echo "error patching gcc"; exit; }
cd ..

#------------------------
# 3. making and install
#------------------------

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

#------------------------
# 4. clean up
#------------------------

rm -Rf $BINUTILS
rm -Rf $GCC
