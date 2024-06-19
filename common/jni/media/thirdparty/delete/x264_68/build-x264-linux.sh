export ARM_ROOT=$PWD/../../../../../../AndroidDevelopTools/Linux/android-ndk-r4b
export ARM_INC=$ARM_ROOT/build/platforms/android-5/arch-arm/usr/include/
export ARM_LIB=$ARM_ROOT/build/platforms/android-5/arch-arm/usr/lib
export ARM_TOOL=$ARM_ROOT/build/prebuilt/linux-x86/arm-eabi-4.4.0
export ARM_LIBO=$ARM_TOOL/lib/gcc/arm-eabi/4.4.0
export PATH=$ARM_TOOL/bin:$PATH
export ARM_PRE=arm-eabi
export X264_GEN=$PWD
export PREBUILT=$ARM_ROOT/build/prebuilt/linux-x86/arm-eabi-4.4.0
export PLATFORM=$ARM_ROOT/build/platforms/android-5/arch-arm

make distclean

./configure --prefix=$X264_GEN \
	--cross-prefix=$PREBUILT/bin/$ARM_PRE- \
	--extra-ldflags="  -nostdlib -Bdynamic  -Wl,--no-undefined -Wl,-z,noexecstack  -Wl,-z,nocopyreloc -Wl,-soname,/system/lib/libz.so -Wl,-rpath-link=$ARM_LIB,-dynamic-linker=/system/bin/linker -L$ARM_LIB -nostdlib $ARM_LIB/crtbegin_dynamic.o $ARM_LIB/crtend_android.o -lc -lm -ldl -lgcc " \
        --extra-ldflags="-Wl,-T,$PREBUILT/arm-eabi/lib/ldscripts/armelf.x -Wl,-rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib -nostdlib $PREBUILT/lib/gcc/arm-eabi/4.4.0/crtbegin.o $PREBUILT/lib/gcc/arm-eabi/4.4.0/crtend.o -lc -lm -ldl" \
    	--extra-cflags="-std=gnu99 -I$ARM_INC -fPIC -DANDROID -fpic -mthumb-interwork -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__  -Wno-psabi -march=armv5te -mtune=xscale -msoft-float -mthumb -Os -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 -DANDROID  -Wa,--noexecstack -MMD -MP " \
	--host=arm-linux
