prefix=/cygdrive/d/QuanTa/Project/QT1.5a/source/backend/jni/media/thirdparty/x264
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
libdir=${exec_prefix}/lib
includedir=${prefix}/include
ARCH=ARM
SYS=LINUX
CC=/cygdrive/d/eclipse/android_ndk/build/prebuilt/windows/arm-eabi-4.4.0/bin/arm-eabi-gcc
CFLAGS=-Wshadow -O3 -fno-fast-math  -Wall -I.  -I/cygdrive/d/eclipse/android_ndk/build/platforms/android-5/arch-arm/usr/include/ -fPIC -DANDROID -fpic -mthumb-interwork -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__  -Wno-psabi -march=armv5te -mtune=xscale -msoft-float -mthumb -Os -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 -DANDROID  -Wa,--noexecstack -MMD -MP  -std=gnu99 -s -fomit-frame-pointer -fno-tree-vectorize
LDFLAGS=   -nostdlib -Bdynamic  -Wl,--no-undefined -Wl,-z,noexecstack  -Wl,-z,nocopyreloc -Wl,-soname,/system/lib/libz.so -Wl,-rpath-link=/cygdrive/d/eclipse/android_ndk/build/platforms/android-5/arch-arm/usr/lib,-dynamic-linker=/system/bin/linker -L/cygdrive/d/eclipse/android_ndk/build/platforms/android-5/arch-arm/usr/lib -nostdlib /cygdrive/d/eclipse/android_ndk/build/platforms/android-5/arch-arm/usr/lib/crtbegin_dynamic.o /cygdrive/d/eclipse/android_ndk/build/platforms/android-5/arch-arm/usr/lib/crtend_android.o -lc -lm -ldl -lgcc  -lm -s
LDFLAGSCLI=
AR=/cygdrive/d/eclipse/android_ndk/build/prebuilt/windows/arm-eabi-4.4.0/bin/arm-eabi-ar
RANLIB=/cygdrive/d/eclipse/android_ndk/build/prebuilt/windows/arm-eabi-4.4.0/bin/arm-eabi-ranlib
STRIP=/cygdrive/d/eclipse/android_ndk/build/prebuilt/windows/arm-eabi-4.4.0/bin/arm-eabi-strip
AS=
ASFLAGS=
EXE=
VIS=no
HAVE_GETOPT_LONG=1
DEVNULL=/dev/null
