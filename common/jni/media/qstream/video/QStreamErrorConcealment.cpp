#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "QStreamH264Parser.h"
#include "QStreamErrorConcealment.h"
#include "logging.h"
#include <sys/time.h>

#define TAG "CQStreamErrorConcealment"
#define LOGV(...) //__logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) //__logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) //__logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) //__logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) //__logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)
#define MAX_FRAME_NUMBER_COUNT                  1023                        // depend on h264 header format

CQStreamErrorConcealment::CQStreamErrorConcealment(void)
{
	pthread_mutexattr_t mutexattr;   // Mutex attribute variable

	// Set the mutex as a recursive mutex
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);

	// create the mutex with the attributes set
	pthread_mutex_init(&mPredictLock, &mutexattr);

	//After initializing the mutex, the thread attribute can be destroyed
	pthread_mutexattr_destroy(&mutexattr);

    Reset();
    mEnableShowError = true;
}

CQStreamErrorConcealment::~CQStreamErrorConcealment(void)
{
    pthread_mutex_destroy(&mPredictLock);
}
void CQStreamErrorConcealment::ShowError(bool enable)
{
    mEnableShowError = enable;
}/* ------------------------------------------
    check is partial frame or losing frame
 
    input: frame_index (current frame index, range: 0~MAX_FRAME_NUMBER_COUNT)
           frame_type (I slice or P slice) 
           isPartialFrame (report from transport layer)
    output: n/a
    return: 0: it's correct frame, 1: partial frame or losing frame case
   ------------------------------------------
*/
//#define IGNORE_UNEXPECTED_ERROR 0
bool CQStreamErrorConcealment::OnErrConcealment(int sliceType,unsigned short frame_index, bool isPartialFrame)
{
    bool result = false;
    pthread_mutex_lock(&mPredictLock);
    if (!isPartialFrame && frame_index>=0)
    {   //Not Partial Frame and Get Frame Number Success
        bool is_disorder = true;
        unsigned short next_pframe_index = mPFramePredictIndex;
        if(IS_ISLICE(sliceType))
        {
            next_pframe_index   = frame_index + 1;
            mIsIFrameNeeded     = false;
            mIsPFramePredicted  = true;
            is_disorder         = false;
        }
        else if (IS_PSLICE(sliceType) && !mIsIFrameNeeded)//P_SLICE_CODE
        {
            is_disorder = (mIsPFramePredicted && ((frame_index != mPFramePredictIndex)));
            if (is_disorder)
            {   //Drop the follwing Gop-run and waiting for next I-Frame
                mIsIFrameNeeded     = true;
                mIsPFramePredicted  = true;
            }else
            {
                next_pframe_index   = frame_index + 1;
                mIsPFramePredicted  = true;
            }
        }
        else if (!mIsIFrameNeeded)//IsBSlice(sliceType) &&
        {   //Ignore BSlice
            LOGD("[VIDEOENGINE]FRAME_CHK_WARNING:Ignore B Slice");
        }

        if (is_disorder && mEnableShowError)
            LOGD("[VIDEOENGINE]FRAME_CHK_FAIL: Slice_type:%d,\tframe_index:%d,\tPredict:%d=>Next:%d",sliceType, frame_index, mPFramePredictIndex,next_pframe_index);

        mPFramePredictIndex = next_pframe_index;
        result = !is_disorder;
    }else if (!isPartialFrame)
    {   //Not partial frame,but failed on get frame number, ignore the error
        //LOGD("FRAME_CHK_WARNING:Failed on Get Frame number"));
        //Drop the follwing Gop-run and waiting for next I-Frame
        mIsIFrameNeeded = true;
        mIsPFramePredicted = true;
        result = false;
        if (mEnableShowError)
        {
            if (result) LOGD("[VIDEOENGINE]FRAME_CHK_WARNING: Failed on Get frame number =>Next:%d",mPFramePredictIndex);
            else LOGD("[VIDEOENGINE]FRAME_CHK_FAIL: Failed on Get frame number =>Next:%d",mPFramePredictIndex);
        }
    }else
    {   //Parial Frame
        //Drop the follwing Gop-run and waiting for next I-Frame
        mIsIFrameNeeded = true;
        mIsPFramePredicted = true;
        result = false;
        
        if (mEnableShowError)
            LOGD("[VIDEOENGINE]FRAME_CHK_FAIL: Paritial Frame =>Next:%d",mPFramePredictIndex);
    } 
    pthread_mutex_unlock(&mPredictLock);
    return result; 
}
void CQStreamErrorConcealment::Reset()
{
    pthread_mutex_lock(&mPredictLock);
    mPFramePredictIndex = 0;
    mIsPFramePredicted = false;
    pthread_mutex_unlock(&mPredictLock);
}
