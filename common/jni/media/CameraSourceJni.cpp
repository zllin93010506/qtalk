/*******************************************************************************
 * All Rights Reserved, Copyright @ Quanta Computer Inc. 2016
 * File Name: OpusEncoderTransformJni.cpp
 * File Mark:
 * Content Description:
 * Other Description: None
 * Version: 1.0
 * Author: Iron Chang
 * Date:
 *
 * History:
 *   1.
 *******************************************************************************/
/* =============================================================================
 Included headers
 ============================================================================= */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <jni.h>
#include "logging.h"
#include "include/CameraSourceJni.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 Macros
 ============================================================================= */
#define TAG "CameraSourceJni"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

static int targetheight = 0;
static int targetwidth = 0;
static int reallength = 0;

static int cameraheight = 0;
static int camerawidth = 0;
static int camerauv = 0;

static int targetuv = 0;
static int diffWidth = 0;
static int diffHeight = 0;

static int mRotation = 0;


void updateCropConfig(int targetWidth, int targetHeight, int realWidth,
		int realHeight, int rotation);
void updateConfig(int realWidth, int realHeight, int rotation);

/* =============================================================================
 Implement Start
 ============================================================================= */
/*
 * Class:     com_quanta_qtalk_media_video_CameraSourcePlus
 * Method:    _init
 * Signature: ()V
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_video_CameraSourcePlus__1init
  (JNIEnv *env, jclass jclass_caller){
	return 0;
}

/*
 * Class:     com_quanta_qtalk_media_video_CameraSourcePlus
 * Method:    _rotate
 * Signature: ([BIII)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_quanta_qtalk_media_video_CameraSourcePlus__1rotate
  (JNIEnv *env, jclass jclass_caller, jbyteArray input, jint realWidth, jint realHeight, jint rotation){
	int inputLength = realWidth * realHeight * 3 / 2;
	jbyte array_input[inputLength];
	jbyte rotated[inputLength];
	jbyteArray output = env->NewByteArray(inputLength);
	env->GetByteArrayRegion(input, 0, inputLength, array_input);

	if(camerawidth!= realWidth || cameraheight!=realHeight ||  mRotation != rotation){
			LOGD("realWidth:%d, realHeight:%d, rotation:%d",realWidth,realHeight,rotation);
			updateConfig(realWidth,realHeight,rotation);
		}


	if (rotation == 270) {
		int y = 0, j = 0;
		int y_limit = cameraheight * camerawidth;
		int y_offsetx = cameraheight * camerawidth;
		int y_diff = 0;
		for (; y < y_limit; y += camerawidth, j++) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx > 0; offsetx -=
					cameraheight, z++) {
				rotated[offsetx - cameraheight + j] = array_input[(z + y)];
			}
		} //Y
		y = 0;
		j = 0;
		int uv_limit = ((cameraheight / 2) * camerawidth);
		int uv_offsetx = camerawidth * cameraheight / 2;
		int uv_diff = 0;
		for (; y < uv_limit; y += camerawidth, j += 2) {
			for (int offsetx = uv_offsetx, z = uv_diff; offsetx > 0; offsetx -=
					cameraheight, z += 2) {
				rotated[camerauv + (offsetx - cameraheight + j)] =
						array_input[camerauv + (z + y)];
				rotated[camerauv + (offsetx - cameraheight + j) + 1] =
						array_input[camerauv + (z + y) + 1];
			}
		} //UV
	} else if (rotation == 180) {
		int y = 0, j = camerawidth * cameraheight - 1;
		int y_limit = cameraheight * camerawidth;
		int y_offsetx = camerawidth;
		int y_diff = 0;
		for (; y < y_limit; y += camerawidth, j -= camerawidth) {
			for (int offsetx = y_offsetx, z = 0; offsetx > 0;
					offsetx--, z++) {
				rotated[offsetx - camerawidth + j] = array_input[(z + y)];
			}
		} //Y
		y = 0;
		j = (camerawidth * cameraheight / 2) - 1;
		int uv_limit = (cameraheight / 2) * camerawidth;
		int uv_offsetx = (camerawidth);
		int uv_diff = 0;
		for (; y < uv_limit; y += camerawidth, j -= camerawidth) {
			for (int offsetx = uv_offsetx, z = 0; offsetx > 0; offsetx -=
					2, z += 2) {
				rotated[camerauv + (offsetx - camerawidth + j) - 1] =
						array_input[camerauv + (z + y)];
				rotated[camerauv + (offsetx - camerawidth + j)] =
						array_input[camerauv + (z + y) + 1];
			}
		} //UV
	} else if (rotation == 0) {
		memcpy(rotated, array_input, inputLength);
	} else {
		int y = 0, j = 0;
		int y_limit = cameraheight * camerawidth;
		int y_offsetx = 0;
		int y_diff = 0;
		for (; y < y_limit; y += camerawidth, j++) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx < camerauv;
					offsetx += cameraheight, z++) {
				rotated[offsetx + cameraheight - 1 - j] = array_input[(z
						+ y)];
			}
		} //Y
		y = 0;
		j = 0;
		int uv_limit = (cameraheight / 2) * camerawidth;
		int uv_offsetx = 0;
		int uv_diff = 0;
		int offsetx_limit = camerauv / 2;
		for (; y < uv_limit; y += camerawidth, j += 2) {
			for (int offsetx = uv_offsetx, z = uv_diff; offsetx < offsetx_limit;
					offsetx += cameraheight, z += 2) {
				rotated[camerauv + (offsetx + cameraheight - 1 - j)] =
						array_input[camerauv + (z + y) + 1];
				rotated[camerauv + (offsetx + cameraheight - 1 - j) - 1] =
						array_input[camerauv + (z + y)];
			}
		} //UV

	}
	env->SetByteArrayRegion(output, 0, inputLength, rotated);
	return output;
}

/*
 * Class:     com_quanta_qtalk_media_video_CameraSourcePlus
 * Method:    _rotateAndtoYuv420SP
 * Signature: ([BIII)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_quanta_qtalk_media_video_CameraSourcePlus__1rotateAndtoYuv420SP
  (JNIEnv *env, jclass jclass_caller, jbyteArray input, jint realWidth, jint realHeight, jint rotation){
	int inputLength = realWidth * realHeight * 3 / 2;
	jbyte array_input[inputLength];
	jbyte rotated[inputLength];
	jbyteArray output = env->NewByteArray(inputLength);
	env->GetByteArrayRegion(input, 0, inputLength, array_input);

	if(camerawidth!= realWidth || cameraheight!=realHeight ||  mRotation != rotation){
			LOGD("realWidth:%d, realHeight:%d, rotation:%d",realWidth,realHeight,rotation);
			updateConfig(realWidth,realHeight,rotation);
		}


	if (rotation == 270) {
		int y = 0, j = 0;
		int y_limit = cameraheight * camerawidth;
		int y_offsetx = cameraheight * camerawidth;
		int y_diff = 0;
		for (; y < y_limit; y += camerawidth, j++) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx > 0; offsetx -=
					cameraheight, z++) {
				rotated[offsetx - cameraheight + j] = array_input[(z + y)];
			}
		} //Y
		y = 0;
		j = 0;
		int uv_limit = ((cameraheight / 2) * camerawidth);
		int uv_offsetx = camerawidth * cameraheight / 2;
		int uv_diff = 0;
		for (; y < uv_limit; y += camerawidth, j += 2) {
			for (int offsetx = uv_offsetx, z = uv_diff; offsetx > 0; offsetx -=
					cameraheight, z += 2) {
				rotated[camerauv + (offsetx - cameraheight + j)] =
						array_input[camerauv + (z + y) + 1];
				rotated[camerauv + (offsetx - cameraheight + j) + 1] =
						array_input[camerauv + (z + y)];
			}
		} //UV
	} else if (rotation == 180) {
		int y = 0, j = camerawidth * cameraheight - 1;
		int y_limit = cameraheight * camerawidth;
		int y_offsetx = camerawidth;
		int y_diff = 0;
		for (; y < y_limit; y += camerawidth, j -= camerawidth) {
			for (int offsetx = y_offsetx, z = 0; offsetx > 0;
					offsetx--, z++) {
				rotated[offsetx - camerawidth + j] = array_input[(z + y)];
			}
		} //Y
		y = 0;
		j = (camerawidth * cameraheight / 2) - 1;
		int uv_limit = (cameraheight / 2) * camerawidth;
		int uv_offsetx = (camerawidth);
		int uv_diff = 0;
		for (; y < uv_limit; y += camerawidth, j -= camerawidth) {
			for (int offsetx = uv_offsetx, z = 0; offsetx > 0; offsetx -=
					2, z += 2) {
				rotated[camerauv + (offsetx - camerawidth + j) - 1] =
						array_input[camerauv + (z + y) + 1];
				rotated[camerauv + (offsetx - camerawidth + j)] =
						array_input[camerauv + (z + y)];
			}
		} //UV
	} else if (rotation == 0) {
		memcpy(rotated, array_input, inputLength);
		for (int i = camerauv; i < inputLength; i += 2) {
			rotated[i] = array_input[i + 1];
			rotated[i + 1] = array_input[i];
		}
	} else {
		int y = 0, j = 0;
		int y_limit = cameraheight * camerawidth;
		int y_offsetx = 0;
		int y_diff = 0;
		for (; y < y_limit; y += camerawidth, j++) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx < camerauv;
					offsetx += cameraheight, z++) {
				rotated[offsetx + cameraheight - 1 - j] = array_input[(z
						+ y)];
			}
		} //Y
		y = 0;
		j = 0;
		int uv_limit = (cameraheight / 2) * camerawidth;
		int uv_offsetx = 0;
		int uv_diff = 0;
		int offsetx_limit = camerauv / 2;
		for (; y < uv_limit; y += camerawidth, j += 2) {
			for (int offsetx = uv_offsetx, z = uv_diff; offsetx < offsetx_limit;
					offsetx += cameraheight, z += 2) {
				rotated[camerauv + (offsetx + cameraheight - 1 - j)] =
						array_input[camerauv + (z + y)];
				rotated[camerauv + (offsetx + cameraheight - 1 - j) - 1] =
						array_input[camerauv + (z + y) + 1];
			}
		} //UV

	}
	env->SetByteArrayRegion(output, 0, inputLength, rotated);
	return output;
}

/*
 * Class:     com_quanta_qtalk_media_video_CameraSourcePlus
 * Method:    _rotateAndtoYuv420
 * Signature: ([BIII)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_quanta_qtalk_media_video_CameraSourcePlus__1rotateAndtoYuv420
  (JNIEnv *env, jclass jclass_caller, jbyteArray input, jint realWidth, jint realHeight, jint rotation){
	int inputLength = realWidth * realHeight * 3 / 2;
	jbyte array_input[inputLength];
	jbyte rotated[inputLength];
	jbyteArray output = env->NewByteArray(inputLength);
	env->GetByteArrayRegion(input, 0, inputLength, array_input);

	if(camerawidth!= realWidth || cameraheight!=realHeight ||  mRotation != rotation){
			LOGD("realWidth:%d, realHeight:%d, rotation:%d",realWidth,realHeight,rotation);
			updateConfig(realWidth,realHeight,rotation);
		}


	if (rotation == 270) {
		int y = 0, j = 0;
		int y_limit = cameraheight * camerawidth;
		int y_offsetx = cameraheight * camerawidth;
		int y_diff = 0;
		for (; y < y_limit; y += camerawidth, j++) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx > 0; offsetx -=
					cameraheight, z++) {
				rotated[offsetx - cameraheight + j] = array_input[(z + y)];
			}
		} //Y
		y = 0;
		j = 0;
		int uv_limit = ((cameraheight / 2) * camerawidth);
		int uv_offsetx = cameraheight * camerawidth / 2;
		int uv_diff = 0;
		int uv_diff_limit = camerawidth;
		int u_size_offset = cameraheight * camerawidth;
		int v_size_offset = cameraheight * camerawidth * 5 / 4;

		for (int z = uv_diff_limit; z>uv_diff; z -= 2) {
			for (int x = y; x < uv_limit; x += camerawidth) {
				rotated[u_size_offset] =
						array_input[camerauv + (z + x)-1];
				rotated[v_size_offset] =
						array_input[camerauv + (z + x)-2];
				u_size_offset++;
				v_size_offset++;
			}
		} //UV
	} else if (rotation == 180) {
		int y = 0, j = camerawidth * cameraheight - 1;
		int y_limit = cameraheight * camerawidth;
		int y_offsetx = camerawidth;
		int y_diff = 0;
		for (; y < y_limit; y += camerawidth, j -= camerawidth) {
			for (int offsetx = y_offsetx, z = 0; offsetx > 0;
					offsetx--, z++) {
				rotated[offsetx - camerawidth + j] = array_input[(z + y)];
			}
		} //Y
		y = (camerawidth * ((cameraheight/2) -1));
		j = (targetwidth * cameraheight / 2) - 1;
		int uv_limit = 0;
		int uv_offsetx = camerawidth;
		int uv_diff =  camerawidth;
		int uv_diff_limit = 0;
		int u_size_offset = cameraheight * camerawidth;
		int v_size_offset = cameraheight * camerawidth * 5 / 4;

		for (int x = y; x >= uv_limit; x -= camerawidth) {
			for (int z = uv_diff; z>uv_diff_limit ; z -= 2) {
				rotated[u_size_offset] =
						array_input[camerauv + (z + x)-1];
				rotated[v_size_offset] =
						array_input[camerauv + (z + x)-2];
				u_size_offset++;
				v_size_offset++;
			}
		} //UV
	} else if (rotation == 0) {
		memcpy(rotated, array_input, inputLength);
		int size = cameraheight*camerawidth;
		int size2 = size*5/4;
		for(int i = 0, j = 0; i<size/4; i++, j+=2)
		{
			rotated[i+size2] = array_input[size+j];   // V
			rotated[i+size]  = array_input[size+j+1]; // U
		}
	} else {
		int y = 0, j = 0;
		int y_limit = cameraheight * camerawidth;
		int y_offsetx = 0;
		int y_diff = 0;
		for (; y < y_limit; y += camerawidth, j++) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx < camerauv;
					offsetx += cameraheight, z++) {
				rotated[offsetx + cameraheight - 1 - j] = array_input[(z
						+ y)];
			}
		} //Y
		y = 0;
		j = 0;
		int uv_limit = ((cameraheight / 2 -1) * camerawidth);
		int uv_offsetx = cameraheight * camerawidth / 2;
		int uv_diff = 0;
		int uv_diff_limit = camerawidth;
		int u_size_offset = cameraheight * camerawidth;
		int v_size_offset = cameraheight * camerawidth * 5 / 4;

		for (int z = uv_diff; z<uv_diff_limit; z += 2) {
			for (int x = uv_limit; x >= y; x -= camerawidth) {
				rotated[u_size_offset] =
						array_input[camerauv + (z + x)+1];
				rotated[v_size_offset] =
						array_input[camerauv + (z + x)];
				u_size_offset++;
				v_size_offset++;
			}
		} //UV

	}
	env->SetByteArrayRegion(output, 0, inputLength, rotated);
	return output;
}

/*
 * Class:     com_quanta_qtalk_media_video_CameraSourcePlus
 * Method:    _rotateAndCrop
 * Signature: (I[BIIIIS)V
 */
JNIEXPORT jbyteArray JNICALL Java_com_quanta_qtalk_media_video_CameraSourcePlus__1rotateAndCrop(
		JNIEnv *env, jclass jclass_caller, jbyteArray input, jint targetWidth,
		jint targetHeight, jint realWidth, jint realHeight, jint rotation) {
	int inputLength = realWidth * realHeight * 3 / 2;
	int outputLenght = targetWidth * targetHeight * 3 / 2;
	jbyte array_input[inputLength];
	jbyte rotatedCropData[outputLenght];
	jbyteArray output = env->NewByteArray(outputLenght);
	env->GetByteArrayRegion(input, 0, inputLength, array_input);

	if(targetwidth!=targetWidth || targetheight!=targetHeight || camerawidth!= realWidth || cameraheight!=realHeight ||  mRotation != rotation){
			LOGD("Config Change, targetwidth:%d",targetwidth);
			updateCropConfig(targetWidth,targetHeight,realWidth,realHeight,rotation);
		}


	if (rotation == 270) {
		int y = camerawidth * diffHeight / 2, j = 0;
		int y_limit = (cameraheight - diffHeight / 2) * camerawidth;
		int y_offsetx = targetheight * targetwidth;
		int y_diff = diffWidth / 2;
		for (; y < y_limit; y += camerawidth, j++) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx > 0; offsetx -=
					targetwidth, z++) {
				rotatedCropData[offsetx - targetwidth + j] = array_input[(z + y)];
			}
		} //Y
		y = (camerawidth * diffHeight / 4);
		j = 0;
		int uv_limit = ((cameraheight / 2 - diffHeight / 4) * camerawidth);
		int uv_offsetx = targetheight * targetwidth / 2;
		int uv_diff = diffWidth / 2;
		for (; y < uv_limit; y += camerawidth, j += 2) {
			for (int offsetx = uv_offsetx, z = uv_diff; offsetx > 0; offsetx -=
					targetwidth, z += 2) {
				rotatedCropData[targetuv + (offsetx - targetwidth + j)] =
						array_input[camerauv + (z + y)];
				rotatedCropData[targetuv + (offsetx - targetwidth + j) + 1] =
						array_input[camerauv + (z + y) + 1];
			}
		} //UV
	} else if (rotation == 180) {
		diffWidth = camerawidth - targetwidth;
		diffHeight = cameraheight - targetheight;
		int y = camerawidth * diffHeight / 2, j = targetwidth * targetheight - 1;
		int y_limit = (cameraheight - diffHeight / 2) * camerawidth;
		int y_offsetx = targetwidth;
		int y_diff = diffWidth / 2;
		for (; y < y_limit; y += camerawidth, j -= targetwidth) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx > 0;
					offsetx--, z++) {
				rotatedCropData[offsetx - targetwidth + j] = array_input[(z + y)];
			}
		} //Y
		y = (camerawidth * diffHeight / 4);
		j = (targetwidth * targetheight / 2) - 1;
		int uv_limit = (cameraheight / 2 - diffHeight / 4) * camerawidth;
		int uv_offsetx = (targetwidth);
		int uv_diff = diffWidth / 2;
		for (; y < uv_limit; y += camerawidth, j -= targetwidth) {
			for (int offsetx = uv_offsetx, z = uv_diff; offsetx > 0; offsetx -=
					2, z += 2) {
				rotatedCropData[targetuv + (offsetx - targetwidth + j) - 1] =
						array_input[camerauv + (z + y)];
				rotatedCropData[targetuv + (offsetx - targetwidth + j)] =
						array_input[camerauv + (z + y) + 1];
			}
		} //UV
	} else if (rotation == 0) {

		if (camerawidth == targetwidth && cameraheight == targetheight) {
			memcpy(rotatedCropData, array_input, inputLength);
		} else {
			diffWidth = camerawidth - targetwidth;
			diffHeight = cameraheight - targetheight;
			int y = camerawidth * diffHeight / 2, j = 0;
			int y_limit = (cameraheight - diffHeight / 2) * camerawidth;
			int y_offsetx = 0;
			int y_diff = diffWidth / 2;
			for (; y < y_limit; y += camerawidth, j += targetwidth) {
				for (int offsetx = 0, z = y_diff; offsetx < targetwidth;
						offsetx++, z++) {
					rotatedCropData[offsetx + j] = array_input[(z + y)];
				}
			} //Y
			y = camerawidth * diffHeight / 4;
			j = 0;
			int uv_limit = (cameraheight / 2 - diffHeight / 4) * camerawidth;
			int uv_offsetx = 0;
			int uv_diff = diffWidth / 2;
			for (; y < uv_limit; y += camerawidth, j += targetwidth) {
				for (int offsetx = uv_offsetx, z = uv_diff; offsetx < targetwidth;
						offsetx += 2, z += 2) {
					rotatedCropData[targetuv + (offsetx + j)] =
							array_input[camerauv + (z + y)];
					rotatedCropData[targetuv + (offsetx + j) + 1] =
							array_input[camerauv + (z + y) + 1];
				}
			} //UV
		}

	} else {
		int y = camerawidth * diffHeight / 2, j = 0;
		int y_limit = (cameraheight - diffHeight / 2) * camerawidth;
		int y_offsetx = 0;
		int y_diff = diffWidth / 2;
		for (; y < y_limit; y += camerawidth, j++) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx < targetuv;
					offsetx += targetwidth, z++) {
				rotatedCropData[offsetx + targetwidth - 1 - j] = array_input[(z
						+ y)];
			}
		} //Y
		y = (camerawidth * diffHeight / 4);
		j = 0;
		int uv_limit = ((cameraheight / 2 - diffHeight / 4) * camerawidth);
		int uv_offsetx = 0;
		int uv_diff = diffWidth / 2;
		int offsetx_limit = targetuv / 2;
		for (; y < uv_limit; y += camerawidth, j += 2) {
			for (int offsetx = uv_offsetx, z = uv_diff; offsetx < offsetx_limit;
					offsetx += targetwidth, z += 2) {
				rotatedCropData[targetuv + (offsetx + targetwidth - 1 - j)] =
						array_input[camerauv + (z + y) + 1];
				rotatedCropData[targetuv + (offsetx + targetwidth - 1 - j) - 1] =
						array_input[camerauv + (z + y)];
			}
		} //UV

	}
	env->SetByteArrayRegion(output, 0, outputLenght, rotatedCropData);
	return output;
}
/*
 * Class:     com_quanta_qtalk_media_video_CameraSourcePlus
 * Method:    _rotateAndCropAndtoYuv420
 * Signature: ([BIIIII)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_quanta_qtalk_media_video_CameraSourcePlus__1rotateAndCropAndtoYuv420SP(
		JNIEnv *env, jclass jclass_caller, jbyteArray input, jint targetWidth,
		jint targetHeight, jint realWidth, jint realHeight, jint rotation) {
	int inputLength = realWidth * realHeight * 3 / 2;
	int outputLenght = targetWidth * targetHeight * 3 / 2;
	jbyte array_input[inputLength];
	jbyte rotatedCropData[outputLenght];
	jbyteArray output = env->NewByteArray(outputLenght);
	env->GetByteArrayRegion(input, 0, inputLength, array_input);

	if(targetwidth!=targetWidth || targetheight!=targetHeight || camerawidth!= realWidth || cameraheight!=realHeight ||  mRotation != rotation){
			updateCropConfig(targetWidth,targetHeight,realWidth,realHeight,rotation);
			LOGD("Config Change, targetwidth:%d",targetwidth);
		}
	if (rotation == 270) {
		int y = camerawidth * diffHeight / 2, j = 0;
		int y_limit = (cameraheight - diffHeight / 2) * camerawidth;
		int y_offsetx = targetheight * targetwidth;
		int y_diff = diffWidth / 2;
		for (; y < y_limit; y += camerawidth, j++) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx > 0; offsetx -=
					targetwidth, z++) {
				rotatedCropData[offsetx - targetwidth + j] = array_input[(z + y)];
			}
		} //Y
		y = (camerawidth * diffHeight / 4);
		j = 0;
		int uv_limit = ((cameraheight / 2 - diffHeight / 4) * camerawidth);
		int uv_offsetx = targetheight * targetwidth / 2;
		int uv_diff = diffWidth / 2;
		for (; y < uv_limit; y += camerawidth, j += 2) {
			for (int offsetx = uv_offsetx, z = uv_diff; offsetx > 0; offsetx -=
					targetwidth, z += 2) {
				rotatedCropData[targetuv + (offsetx - targetwidth + j)] =
						array_input[camerauv + (z + y) + 1];
				rotatedCropData[targetuv + (offsetx - targetwidth + j) + 1] =
						array_input[camerauv + (z + y)];
			}
		} //UV
	} else if (rotation == 180) {
		diffWidth = camerawidth - targetwidth;
		diffHeight = cameraheight - targetheight;
		int y = camerawidth * diffHeight / 2, j = targetwidth * targetheight - 1;
		int y_limit = (cameraheight - diffHeight / 2) * camerawidth;
		int y_offsetx = targetwidth;
		int y_diff = diffWidth / 2;
		for (; y < y_limit; y += camerawidth, j -= targetwidth) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx > 0;
					offsetx--, z++) {
				rotatedCropData[offsetx - targetwidth + j] = array_input[(z + y)];
			}
		} //Y
		y = (camerawidth * diffHeight / 4);
		j = (targetwidth * targetheight / 2) - 1;
		int uv_limit = (cameraheight / 2 - diffHeight / 4) * camerawidth;
		int uv_offsetx = (targetwidth);
		int uv_diff = diffWidth / 2;
		for (; y < uv_limit; y += camerawidth, j -= targetwidth) {
			for (int offsetx = uv_offsetx, z = uv_diff; offsetx > 0; offsetx -=
					2, z += 2) {
				rotatedCropData[targetuv + (offsetx - targetwidth + j) - 1] =
						array_input[camerauv + (z + y) + 1];
				rotatedCropData[targetuv + (offsetx - targetwidth + j)] =
						array_input[camerauv + (z + y)];
			}
		} //UV
	} else if (rotation == 0) {

		if (camerawidth == targetwidth && cameraheight == targetheight) {
			memcpy(rotatedCropData, array_input, inputLength);
			for (int i = targetuv; i < inputLength; i += 2) {
				rotatedCropData[i] = array_input[i + 1];
				rotatedCropData[i + 1] = array_input[i];
			}
		} else {
			diffWidth = camerawidth - targetwidth;
			diffHeight = cameraheight - targetheight;
			int y = camerawidth * diffHeight / 2, j = 0;
			int y_limit = (cameraheight - diffHeight / 2) * camerawidth;
			int y_offsetx = 0;
			int y_diff = diffWidth / 2;
			for (; y < y_limit; y += camerawidth, j += targetwidth) {
				for (int offsetx = 0, z = y_diff; offsetx < targetwidth;
						offsetx++, z++) {
					rotatedCropData[offsetx + j] = array_input[(z + y)];
				}
			} //Y
			y = camerawidth * diffHeight / 4;
			j = 0;
			int uv_limit = (cameraheight / 2 - diffHeight / 4) * camerawidth;
			int uv_offsetx = 0;
			int uv_diff = diffWidth / 2;
			for (; y < uv_limit; y += camerawidth, j += targetwidth) {
				for (int offsetx = uv_offsetx, z = uv_diff; offsetx < targetwidth;
						offsetx += 2, z += 2) {
					rotatedCropData[targetuv + (offsetx + j)] =
							array_input[camerauv + (z + y)];
					rotatedCropData[targetuv + (offsetx + j) + 1] =
							array_input[camerauv + (z + y) + 1];
				}
			} //UV
		}

	} else {
		int y = camerawidth * diffHeight / 2, j = 0;
		int y_limit = (cameraheight - diffHeight / 2) * camerawidth;
		int y_offsetx = 0;
		int y_diff = diffWidth / 2;
		for (; y < y_limit; y += camerawidth, j++) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx < targetuv;
					offsetx += targetwidth, z++) {
				rotatedCropData[offsetx + targetwidth - 1 - j] = array_input[(z
						+ y)];
			}
		} //Y
		y = (camerawidth * diffHeight / 4);
		j = 0;
		int uv_limit = ((cameraheight / 2 - diffHeight / 4) * camerawidth);
		int uv_offsetx = 0;
		int uv_diff = diffWidth / 2;
		int offsetx_limit = targetuv / 2;
		for (; y < uv_limit; y += camerawidth, j += 2) {
			for (int offsetx = uv_offsetx, z = uv_diff; offsetx < offsetx_limit;
					offsetx += targetwidth, z += 2) {
				rotatedCropData[targetuv + (offsetx + targetwidth - 1 - j)] =
						array_input[camerauv + (z + y)];
				rotatedCropData[targetuv + (offsetx + targetwidth - 1 - j) - 1] =
						array_input[camerauv + (z + y) + 1];
			}
		} //UV

	}
	env->SetByteArrayRegion(output, 0, outputLenght, rotatedCropData);
	return output;
}

/*
 * Class:     com_quanta_qtalk_media_video_CameraSourcePlus
 * Method:    _rotateAndCropAndtoYuv420
 * Signature: ([BIIIII)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_quanta_qtalk_media_video_CameraSourcePlus__1rotateAndCropAndtoYuv420(
		JNIEnv *env, jclass jclass_caller, jbyteArray input, jint targetWidth,
		jint targetHeight, jint realWidth, jint realHeight, jint rotation) {
	int inputLength = realWidth * realHeight * 3 / 2;
	int outputLenght = targetWidth * targetHeight * 3 / 2;
	jbyte array_input[inputLength];
	jbyte rotatedCropData[outputLenght];
	jbyteArray output = env->NewByteArray(outputLenght);
	env->GetByteArrayRegion(input, 0, inputLength, array_input);


	if(targetwidth!=targetWidth || targetheight!=targetHeight || camerawidth!= realWidth || cameraheight!=realHeight ||  mRotation != rotation){
		LOGD("Config Change");
		updateCropConfig(targetWidth,targetHeight,realWidth,realHeight,rotation);
	}

	if (rotation == 270) {
		int y = camerawidth * diffHeight / 2, j = 0;
		int y_limit = (cameraheight - diffHeight / 2) * camerawidth;
		int y_offsetx = targetheight * targetwidth;
		int y_diff = diffWidth / 2;
		for (; y < y_limit; y += camerawidth, j++) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx > 0; offsetx -=
					targetwidth, z++) {
				rotatedCropData[offsetx - targetwidth + j] = array_input[(z + y)];
			}
		} //Y
		y = (camerawidth * diffHeight / 4);
		j = 0;
		int uv_limit = ((cameraheight / 2 - diffHeight / 4) * camerawidth);
		int uv_offsetx = targetheight * targetwidth / 2;
		int uv_diff = diffWidth / 2;
		int uv_diff_limit = diffWidth / 2 + targetheight;
		int u_size_offset = targetheight * targetwidth;
		int v_size_offset = targetheight * targetwidth * 5 / 4;

		for (int z = uv_diff_limit; z>uv_diff; z -= 2) {
			for (int x = y; x < uv_limit; x += camerawidth) {
				rotatedCropData[u_size_offset] =
						array_input[camerauv + (z + x)-1];
				rotatedCropData[v_size_offset] =
						array_input[camerauv + (z + x)-2];
				u_size_offset++;
				v_size_offset++;
			}
		} //UV
	} else if (rotation == 180) {
		diffWidth = camerawidth - targetwidth;
		diffHeight = cameraheight - targetheight;
		int y = camerawidth * diffHeight / 2, j = targetwidth * targetheight - 1;
		int y_limit = (cameraheight - diffHeight / 2) * camerawidth;
		int y_offsetx = targetwidth;
		int y_diff = diffWidth / 2;
		for (; y < y_limit; y += camerawidth, j -= targetwidth) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx > 0;
					offsetx--, z++) {
				rotatedCropData[offsetx - targetwidth + j] = array_input[(z + y)];
			}
		} //Y
		y = (camerawidth * ((targetheight+(diffHeight/2))/2 -1));
		j = (targetwidth * targetheight / 2) - 1;
		int uv_limit = (diffHeight / 2) * camerawidth;
		int uv_offsetx = (targetwidth);
		int uv_diff = diffWidth / 2 + targetwidth;
		int uv_diff_limit = diffWidth / 2;
		int u_size_offset = targetheight * targetwidth;
		int v_size_offset = targetheight * targetwidth * 5 / 4;

		for (int x = y; x >= uv_limit; x -= camerawidth) {
			for (int z = uv_diff; z>uv_diff_limit ; z -= 2) {
				rotatedCropData[u_size_offset] =
						array_input[camerauv + (z + x)-1];
				rotatedCropData[v_size_offset] =
						array_input[camerauv + (z + x)-2];

				u_size_offset++;
				v_size_offset++;

			}
		} //UV
	} else if (rotation == 0) {

		if (camerawidth == targetwidth && cameraheight == targetheight) {
			memcpy(rotatedCropData, array_input, inputLength);
		    int size = targetheight*targetwidth;
		    int size2 = size*5/4;
		    for(int i = 0, j = 0; i<size/4; i++, j+=2)
		    {
		    	rotatedCropData[i+size2] = array_input[size+j];   // V
		    	rotatedCropData[i+size]  = array_input[size+j+1]; // U
		    }
		} else {
			diffWidth = camerawidth - targetwidth;
			diffHeight = cameraheight - targetheight;
			int y = camerawidth * diffHeight / 2, j = 0;
			int y_limit = (cameraheight - diffHeight / 2) * camerawidth;
			int y_offsetx = 0;
			int y_diff = diffWidth / 2;
			for (; y < y_limit; y += camerawidth, j += targetwidth) {
				for (int offsetx = 0, z = y_diff; offsetx < targetwidth;
						offsetx++, z++) {
					rotatedCropData[offsetx + j] = array_input[(z + y)];
				}
			} //Y
			y = camerawidth * diffHeight / 4;
			j = 0;
			int uv_limit = (cameraheight / 2 - diffHeight / 4) * camerawidth;
			int uv_offsetx = 0;
			int uv_diff = diffWidth / 2;
			int uv_diff_limit = diffWidth / 2 + targetheight;
			int u_size_offset = targetheight * targetwidth;
			int v_size_offset = targetheight * targetwidth * 5 / 4;

			for (int x = y; x <= uv_limit; x += camerawidth) {
				for (int z = uv_diff; z<diffWidth / 2 + targetwidth; z += 2) {
					rotatedCropData[u_size_offset] =
							array_input[camerauv + (z + x)+1];
					rotatedCropData[v_size_offset] =
							array_input[camerauv + (z + x)];
					u_size_offset++;
					v_size_offset++;
				}
			} //UV
		}

	} else {
		int y = camerawidth * diffHeight / 2, j = 0;
		int y_limit = (cameraheight - diffHeight / 2) * camerawidth;
		int y_offsetx = 0;
		int y_diff = diffWidth / 2;
		for (; y < y_limit; y += camerawidth, j++) {
			for (int offsetx = y_offsetx, z = y_diff; offsetx < targetuv;
					offsetx += targetwidth, z++) {
				rotatedCropData[offsetx + targetwidth - 1 - j] = array_input[(z
						+ y)];
			}
		} //Y
		y = (camerawidth * diffHeight / 4);
		j = 0;
		int uv_limit = ((cameraheight / 2 - diffHeight / 4) * camerawidth);
		int uv_offsetx = targetheight * targetwidth / 2;
		int uv_diff = diffWidth / 2;
		int uv_diff_limit = diffWidth / 2 + targetheight;
		int u_size_offset = targetheight * targetwidth;
		int v_size_offset = targetheight * targetwidth * 5 / 4;

		for (int z = uv_diff; z<uv_diff_limit; z += 2) {
			for (int x = uv_limit; x > y; x -= camerawidth) {
				rotatedCropData[u_size_offset] =
						array_input[camerauv + (z + x)-1];
				rotatedCropData[v_size_offset] =
						array_input[camerauv + (z + x)-2];
				u_size_offset++;
				v_size_offset++;
			}
		} //UV

	}
	env->SetByteArrayRegion(output, 0, outputLenght, rotatedCropData);
	return output;
}
void updateCropConfig(int targetWidth, int targetHeight, int realWidth,
		int realHeight, int rotation) {

	targetheight = targetHeight, targetwidth = targetWidth;
	reallength = realWidth * realHeight * 3 / 2;
	cameraheight = realHeight;
	camerawidth = realWidth;
	camerauv = cameraheight * camerawidth;

	targetuv = targetwidth * targetheight;
	diffWidth = camerawidth - targetheight;
	diffHeight = cameraheight - targetwidth;

	mRotation = rotation;

}
void updateConfig(int realWidth, int realHeight, int rotation) {

	reallength = realWidth * realHeight * 3 / 2;
	cameraheight = realHeight;
	camerawidth = realWidth;
	camerauv = cameraheight * camerawidth;

	mRotation = rotation;

}

#ifdef __cplusplus
}
#endif

