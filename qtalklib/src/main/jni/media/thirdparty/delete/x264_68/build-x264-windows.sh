export ARM_ROOT=/cygdrive/d/android-ndk-r4b-windows/android-ndk-r4b
export ARM_INC=$ARM_ROOT/build/platforms/android-5/arch-arm/usr/include/
export ARM_LIB=$ARM_ROOT/build/platforms/android-5/arch-arm/usr/lib
export ARM_TOOL=$ARM_ROOT/build/prebuilt/windows/arm-eabi-4.4.0
export ARM_LIBO=$ARM_TOOL/lib/gcc/arm-eabi/4.4.0
export PATH=$ARM_TOOL/bin:$PATH
export ARM_PRE=arm-eabi
export X264_GEN=/cygdrive/d/Temp_job/QT1.5A_X264-0912/jni/x264/x264_link
export PREBUILT=/cygdrive/d/android-ndk-r4b-windows/android-ndk-r4b/build/prebuilt/windows/arm-eabi-4.4.0
export PLATFORM=/cygdrive/d/android-ndk-r4b-windows/android-ndk-r4b/build/platforms/android-8/arch-arm

./configure --prefix=$X264_GEN \
        --disable-gpac \
        --extra-cflags="-std=gnu99 -I$ARM_INC -fPIC -DANDROID -fpic -mthumb-interwork -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__  -Wno-psabi -march=armv5te -mtune=xscale -msoft-float -mthumb -Os -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 -DANDROID  -Wa,--noexecstack -MMD -MP " \
        --extra-ldflags="  -nostdlib -Bdynamic  -Wl,--no-undefined -Wl,-z,noexecstack  -Wl,-z,nocopyreloc -Wl,-soname,/system/lib/libz.so -Wl,-rpath-link=$ARM_LIB,-dynamic-linker=/system/bin/linker -L$ARM_LIB -nostdlib $ARM_LIB/crtbegin_dynamic.o $ARM_LIB/crtend_android.o -lc -ldl -lgcc " \
	--cross-prefix=$ARM_TOOL/bin/${ARM_PRE}- \
        --disable-asm \
        --disable-pthread  \
        --host=arm-linux