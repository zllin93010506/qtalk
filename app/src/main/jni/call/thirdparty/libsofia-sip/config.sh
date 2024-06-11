cd sofia-sip-1.12.11
export NDKROOT=/home/steven/MyWork/qt1.5a/AndroidDevelopTools/Linux/android-ndk-r5b
export SYSROOT=$NDKROOT/platforms/android-5/arch-arm/


export RESULT=$PWD/../sofialib
rm -rf $RESULT

#generate toolchain
$NDKROOT/build/tools/make-standalone-toolchain.sh --platform=android-5 --install-dir=/tmp/my-android-toolchain

#export the toolchain path to path
export PATH=/tmp/my-android-toolchain/bin:$PATH
export CC=arm-linux-androideabi-gcc

make clean
./configure \
	--host=arm-linux-androideabi \
        CFLAGS='-march=armv7-a -mfloat-abi=softfp' \
        LDFLAGS='-Wl,--fix-cortex-a8' \
        LIBS="-lc "\
        --without-glib
make
mkdir $RESULT
find -type f -name "*.a" -exec cp {} $RESULT/ \;
