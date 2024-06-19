prefix=./out/armeabi-v7a
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
libdir=${exec_prefix}/lib
includedir=${prefix}/include
ARCH=ARM
SYS=LINUX
CC=/home/frank/eclipse/ndk/android-ndk-r9c/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin/arm-linux-androideabi-gcc
CFLAGS=-Wshadow -O3 -fno-fast-math  -Wall -I. --sysroot=/home/frank/eclipse/ndk/android-ndk-r9c/platforms/android-12/arch-arm -std=gnu99 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -fPIC -s -fomit-frame-pointer -fno-tree-vectorize
LDFLAGS= --sysroot=/home/frank/eclipse/ndk/android-ndk-r9c/platforms/android-12/arch-arm -lm -Wl,-Bsymbolic -s
LDFLAGSCLI=
AR=/home/frank/eclipse/ndk/android-ndk-r9c/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin/arm-linux-androideabi-ar
RANLIB=/home/frank/eclipse/ndk/android-ndk-r9c/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin/arm-linux-androideabi-ranlib
STRIP=/home/frank/eclipse/ndk/android-ndk-r9c/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin/arm-linux-androideabi-strip
AS=/home/frank/eclipse/ndk/android-ndk-r9c/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin/arm-linux-androideabi-gcc
ASFLAGS=  -Wall -I. --sysroot=/home/frank/eclipse/ndk/android-ndk-r9c/platforms/android-12/arch-arm -std=gnu99 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -c -DPIC
EXE=
VIS=no
HAVE_GETOPT_LONG=1
DEVNULL=/dev/null
GPL=yes
