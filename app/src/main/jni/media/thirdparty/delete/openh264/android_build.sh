#!/bin/bash
export NDK=/home/frank/Android/Sdk/ndk/android-ndk-r14b # Change me
export JNILIBS_DIR=./android/lib # Change me
export INCLUDE_DIR=./android/include # Change me
export DOWNLOAD_URL=https://github.com/cisco/openh264/archive/master.zip
export OPENH264_SRC=./openh264-2.2.0

mkdir -p tmp
ln -sf $NDK/prebuilt/linux-x86_64/bin/yasm tmp/nasm
export PATH=$(pwd)/tmp:$PATH

#if [ ! -f src.zip ]; then
#	curl -L $DOWNLOAD_URL --output src.zip || exit 1
#fi

declare -A arch_abis=(
	["arm"]="armeabi-v7a"
	["arm64"]="arm64-v8a"
	["x86"]="x86"
	["x86_64"]="x86_64"
)

declare -A ndk_levels=(
	["arm"]="16"
	["arm64"]="21"
	["x86"]="16"
	["x86_64"]="21"
)

for arch in arm arm64 x86 x86_64; do
	echo "Building for $arch (${arch_abis[$arch]})"
	#unzip src.zip || exit 1
	#rm -rf src
	#mv openh264-master src
	if [ "$arch" == "x86" ]; then
		export ASMFLAGS=-DX86_32_PICASM
	else
		export ASMFLAGS=
	fi
    make OS=android NDKROOT=$NDK TARGET=android-${ndk_levels[$arch]} NDKLEVEL=${ndk_levels[$arch]} ARCH=$arch -C $OPENH264_SRC NDK_TOOLCHAIN_VERSION=clang clean
	make OS=android NDKROOT=$NDK TARGET=android-${ndk_levels[$arch]} NDKLEVEL=${ndk_levels[$arch]} ARCH=$arch -C $OPENH264_SRC NDK_TOOLCHAIN_VERSION=clang libraries || exit 1
	mkdir -p "$JNILIBS_DIR/${arch_abis[$arch]}"
	cp -v $OPENH264_SRC/libopenh264.{so,a} "$JNILIBS_DIR/${arch_abis[$arch]}"
done

cp -v $OPENH264_SRC/codec/api/svc/*.h "$INCLUDE_DIR/"

#rm -rf tmp src
