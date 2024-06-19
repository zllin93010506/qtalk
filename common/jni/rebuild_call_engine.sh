rm ../obj/local/armeabi/objs/call_engine/  -rf
rm ../obj
rm ../libs/armeabi-v7a/ -rf
rm ../libs/x86 -rf

ndk-build -j4
