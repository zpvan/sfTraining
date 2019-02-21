//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party's software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party's software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar's confidential information and you agree to keep MStar's
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer's product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>

//#define LOG_NDEBUG 0
#define LOG_TAG "PlayerInterface"
#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <gui/Surface.h>
#include <media/mediaplayer.h>
#include <media/AudioTrack.h>
#include <gui/Surface.h>
#include <binder/MemoryBase.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <system/audio.h>
#include <system/window.h>
#include "media/PlayerInterface.h"
#include <cutils/properties.h>


namespace android {

InterfaceListener::InterfaceListener(PlayerInterface *Player)
{
    mPlayer = Player;
}

InterfaceListener::~InterfaceListener()
{

}

void InterfaceListener::notify(int msg, int ext1, int ext2, const Parcel *obj)
{
    mPlayer->MsgNotify(msg,ext1,ext2,obj);
}

PlayerInterface::PlayerInterface()
{
    ALOGD("constructor\n");
    mListener = NULL;
    mDataSource = NULL;
	mDrmStream = false;
    mStartTime = -1;
    mPercentRightNow = 100;
    mePlayingState = E_MM_INTERFACE_STATE_NULL;
    mCallback = NULL;
    mpApInstance = NULL;
    mLockPiThreadId = 0;
    mMyVector.clear();
    if (mThread.get()) {
        mThread->requestExitAndWait();
        mThread.clear();
    }
    mMediaPlayer = NULL;
    mNotifyListener = NULL;
	mDropErrMode = 0;
    mThread = new MonitorThread(this);
    mThread->run("Moitor Thread");
}

PlayerInterface::~PlayerInterface()
{
    ALOGD("destructor\n");
    if(mMediaPlayer.get())
    {
        mMediaPlayer->stop();
        mMediaPlayer->disconnect();
        mMediaPlayer.clear();
    }
    if (mThread.get()) {
        mThread->requestExitAndWait();
        mThread.clear();
    }
    if (mListener != NULL) {
        mListener.clear();
        mListener = NULL;
    }
    if (mDataSource != NULL) {
        mDataSource.clear();
        mDataSource = NULL;
    }

}

void PlayerInterface::MsgNotify(int msg, int ext1, int ext2, const Parcel *obj)
{
    bool locked = false;

    ALOGD("MsgNotify: %d, %d, %d, getThreadId = %p, mLockPiThreadId = %p\n",msg, ext1, ext2,getThreadId,mLockPiThreadId);

    if (mLockPiThreadId != getThreadId()) {
        mLock.lock();
        locked = true;
    }

    switch (msg) {
    case MEDIA_PREPARED:
        if (ext1 == MEDIA_PREPARED) {
            if (mCallback != NULL) {
                (*mCallback)(E_MM_INTERFACE_START_PLAY, E_MM_INTERFACE_START_PLAY,0 ,mpApInstance);
            }
            if (mNotifyListener != NULL)
            {
                mNotifyListener->Notify(E_MM_INTERFACE_START_PLAY, E_MM_INTERFACE_START_PLAY,0 ,mpApInstance);
            }
        } else {
            if (mCallback != NULL) {
                (*mCallback)(E_MM_INTERFACE_START_PLAY, 0, 0, mpApInstance);
            }
            if (mNotifyListener != NULL)
            {
                mNotifyListener->Notify(E_MM_INTERFACE_START_PLAY, 0, 0, mpApInstance);
            }
            mMyVector.push_back(E_MM_INTERFACE_STATE_INIT_DONE);
        }
        break;

    case MEDIA_PLAYBACK_COMPLETE:
        if (mCallback != NULL) {
            (*mCallback)(E_MM_INTERFACE_EXIT_OK, 0, 0, mpApInstance);
        }
        if (mNotifyListener != NULL)
        {
            mNotifyListener->Notify(E_MM_INTERFACE_EXIT_OK, 0, 0, mpApInstance);
        }
        mMyVector.push_back(E_MM_INTERFACE_STATE_PLAY_DONE);
        break;

    case MEDIA_INFO:
        {
            switch (ext1) {
            case MEDIA_INFO_BUFFERING_START:
                if (mPercentRightNow == 100) {
                    if (mCallback != NULL) {
                        (*mCallback)(E_MM_INTERFACE_INSUFFICIENT_DATA, 0, 0, mpApInstance);
                    }
                    //mMyVector.push_back(E_MM_INTERFACE_STATE_DATA_INSUFFICENT);
                }
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_INSUFFICIENT_DATA, 0, 0, mpApInstance);
                }
                mPercentRightNow = ext2;
                break;

            case MEDIA_INFO_BUFFERING_END:
                mPercentRightNow = 100;
                if (mCallback != NULL) {
                    (*mCallback)(E_MM_INTERFACE_SUFFICIENT_DATA, 0, 0, mpApInstance);
                }
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_SUFFICIENT_DATA, 0, 0, mpApInstance);
                }
                //mMyVector.push_back(E_MM_INTERFACE_STATE_DATA_SUFFICENT);
                break;

            case MEDIA_INFO_RENDERING_START:
                if (mCallback != NULL) {
                    (*mCallback)(E_MM_INTERFACE_RENDERING_START, 0, 0, mpApInstance);
                }
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_RENDERING_START, 0, 0, mpApInstance);
                }
                break;
			case MEDIA_INFO_AUDIO_UNSUPPORT:
				if (mCallback != NULL) {
                    (*mCallback)(E_MM_INTERFACE_AUDIO_UNSUPPORT, 0, 0, mpApInstance);
                }
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_AUDIO_UNSUPPORT, 0, 0, mpApInstance);
                }
                break;
			case MEDIA_INFO_DROP_FRAME:
				if (mCallback != NULL) {
                    (*mCallback)(E_MM_INTERFACE_DROP_FRAME, 0, 0, mpApInstance);
                }
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_DROP_FRAME, 0, 0, mpApInstance);
                }
				break;
            case MEDIA_INFO_AV_OUT_OF_SYNC:
                if (mCallback != NULL) {
                    (*mCallback)(E_MM_INTERFACE_AV_OUT_OF_SYNC, 0, 0, mpApInstance);
                }
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_AV_OUT_OF_SYNC, 0, 0, mpApInstance);
                }
                break;
            case MEDIA_INFO_PTS_INVALID:
                if (mCallback != NULL) {
                    (*mCallback)(E_MM_INTERFACE_PTS_INVALID, 0, 0, mpApInstance);
                }
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_PTS_INVALID, 0, 0, mpApInstance);
                }
                break;
            case MEDIA_INFO_VIDEO_DECODE_ERROR:
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_VIDEO_DECODE_ERROR, 0, 0, mpApInstance);
                }
                break;
            case MEDIA_INFO_VIDEO_DECODE_OUT_OF_MEMORY:
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_VIDEO_DECODE_OUT_OF_MEMORY, 0, 0, mpApInstance);
                }
                break;
            case MEDIA_INFO_VIDEO_PTS_ERROR:
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_VIDEO_PTS_ERROR, 0, 0, mpApInstance);
                }
                break;
            case MEDIA_INFO_AUDIO_DECODE_ERROR:
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_AUDIO_DECODE_ERROR, 0, 0, mpApInstance);
                }
                break;
            case MEDIA_INFO_AUDIO_DECODE_SKIP_FRAME:
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_AUDIO_DECODE_SKIP_FRAME, 0, 0, mpApInstance);
                }
                break;
            case MEDIA_INFO_AUDIO_DECODE_OUT_OF_MEMORY:
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_AUDIO_DECODE_OUT_OF_MEMORY, 0, 0, mpApInstance);
                }
                break;
            case MEDIA_INFO_AUDIO_PTS_ERROR:
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_AUDIO_PTS_ERROR, 0, 0, mpApInstance);
                }
                break;
            case MEDIA_INFO_VIDEO_IS_INTERLACE:
                if (mNotifyListener != NULL)
                {
                    mNotifyListener->Notify(E_MM_INTERFACE_VIDEO_INTERLACE, 0, ext2, mpApInstance);
                }
                break;
            default:
                break;
            }
        }
        break;
    case MEDIA_SET_VIDEO_SIZE:
        {
            if(mNotifyListener != NULL)
            {
                mNotifyListener->Notify(E_MM_INTERFACE_VIDEO_SIZE, ext1, ext2, mpApInstance);
            }
        }
        break;

    case MEDIA_ERROR:
        {
            if (mCallback != NULL) {
                (*mCallback)(E_MM_INTERFACE_EXIT_ERROR, ext1, ext2, mpApInstance);
            }
            if(mNotifyListener != NULL)
            {
                mNotifyListener->Notify(E_MM_INTERFACE_EXIT_ERROR, ext1, ext2, mpApInstance);
            }
            mMyVector.push_back(E_MM_INTERFACE_STATE_PLAY_DONE);
        }
        break;
    default:
        break;
    }

    if (locked) mLock.unlock();

}


status_t PlayerInterface::SetDataSource(ST_MM_INTERFACE_DATASOURCE_INFO* pstSourceInfo,sp <BaseDataSource> DataSource)
{
    status_t err = OK;
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    ALOGD("SetDataSource: %d, %d\n",pstSourceInfo->eSeekableMode, pstSourceInfo->eStreamType);

    if (mListener == NULL) {
        mListener = new InterfaceListener(this);
    }
    memset(&mstSourceInfo, 0,sizeof(ST_MS_DATASOURCE_INFO));
    mstSourceInfo.ePlayMode= (EN_MS_DATASOURCE_PLAYER_TYPE)pstSourceInfo->eStreamType;
    mstSourceInfo.eAppMode= (EN_MS_DATASOURCE_APP_MODE)pstSourceInfo->eAppMode;
    mstSourceInfo.bUseRingBuff = pstSourceInfo->bUseRingBuff;
    mstSourceInfo.bPC2TVMode   = pstSourceInfo->bPC2TVMode;
    mstSourceInfo.bAutoPauseResume = pstSourceInfo->bAutoPauseResume;

    switch ( pstSourceInfo->eSeekableMode) {
    case E_MM_INTERFACE_SEEKABLE_MODE_WITH_SEEK:
        mstSourceInfo.eContentType = E_DATASOURCE_CONTENT_TYPE_MASS_STORAGE;
        break;

    case E_MM_INTERFACE_SEEKABLE_MODE_WITHOUT_SEEK:
        mstSourceInfo.eContentType = E_DATASOURCE_CONTENT_TYPE_NETWORK_STREAM_WITHOUT_SEEK;
        break;

    case E_MM_INTERFACE_SEEKABLE_MODE_ES:
        mstSourceInfo.eContentType = E_DATASOURCE_CONTENT_TYPE_ES;
        break;

    default:
        break;
    }
    mstSourceInfo.eESVideoCodec = (EN_MS_DATASOURCE_ES_VIDEO_CODEC)(pstSourceInfo->eESVideoCodec);
    mstSourceInfo.eESAudioCodec = (EN_MS_DATASOURCE_ES_AUDIO_CODEC)(pstSourceInfo->eESAudioCodec);
    mstSourceInfo.eMediaFormat = (EN_MS_DATASOURCE_MEDIA_FORMAT_TYPE)(pstSourceInfo->eMediaFormat);
    mstSourceInfo.eAppType = (EN_MS_DATASOURCE_AP_TYPE)(pstSourceInfo->eAppType);
    mstSourceInfo.ePlayerType = (EN_MS_PLAYER_TYPE)(pstSourceInfo->ePlayerType);
    mstSourceInfo.u32TeeEncryptFlag = pstSourceInfo->u32TeeEncryptFlag;

    ALOGE("codec: %d, %d", (int) mstSourceInfo.eESVideoCodec, (int) mstSourceInfo.eESAudioCodec);

    mDataSource = DataSource;
    if(mMediaPlayer.get() == NULL)
    {
        mMediaPlayer = new MediaPlayer();
        err = mMediaPlayer->setListener(mListener);
    }
    mLockPiThreadId = 0;
    return err;
}

status_t PlayerInterface::SetSurface(const sp<IGraphicBufferProducer>& surfaceTexture)
{
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    msurfaceTexture = surfaceTexture;
    status_t sta =  mMediaPlayer->setVideoSurfaceTexture(surfaceTexture);
    mLockPiThreadId = 0;
    return sta;
}

void PlayerInterface::RegisterCallBack(CmdCallback pCmdCbFunc, void* pInstance)
{
    mCallback = pCmdCbFunc;
    mpApInstance = pInstance;
}

status_t PlayerInterface::Play(char* url)
{
    status_t err = OK;
	Parcel parcel;
    ALOGD("play: %s\n",url);
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);

    if (mMediaPlayer.get() == NULL)
    {
        mMediaPlayer = new MediaPlayer();
        err = mMediaPlayer->setListener(mListener);
    }
    mMediaPlayer->setDataSourceInfo(mstSourceInfo);

    if (mDataSource == NULL) {
        err = UNKNOWN_ERROR;
        ALOGD("No Data Source!!\n");
        goto FUNC_END;
    } else {
            err = mMediaPlayer->setDataSource(mDataSource);
    }

    if (err != OK) {
        ALOGD("setDataSource error\n");
        goto FUNC_END;
    }

    if (mDrmStream) {
		parcel.writeInt64(1);
    } else {
        parcel.writeInt64(0);
    }

    mMediaPlayer->setParameter(KEY_PARAMETER_SET_TELECOM_HANDLE,parcel);

    {
        Parcel request;
        char value[256];
        int mode = (int)E_MM_INTERFACE_SEAMLESS_DS_FREEZE;
        if (property_get("mstar.media.seamlessmode", value, NULL))
        {
            mode = atoi(value);
        }
        request.writeInt32(mode);
        request.setDataPosition(0);
        mMediaPlayer->setParameter(KEY_PARAMETER_SET_SEAMLESS_MODE, request);
    }

    if (msurfaceTexture != NULL) {
        mMediaPlayer->setVideoSurfaceTexture(msurfaceTexture);
    }
    err = mMediaPlayer->prepareAsync();
	if (msubSurfaceTexture != NULL) {
        mMediaPlayer->setSubtitleSurface(msubSurfaceTexture);
    }
    mePlayingState = E_MM_INTERFACE_STATE_WAIT_INIT;
    mLockPiThreadId = 0;
    FUNC_END:
    return err;
}

void PlayerInterface::setDrmStream()
{
     mDrmStream = true;
}

status_t PlayerInterface::Stop()
{
    ALOGD("Stop\n");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() != NULL)
    {
        mMediaPlayer->stop();
        mMediaPlayer->disconnect();
        mMediaPlayer.clear();
        msurfaceTexture.clear();
        msubSurfaceTexture.clear();
    }
    mDropErrMode = 0;
    mDrmStream = false;
    mePlayingState = E_MM_INTERFACE_STATE_NULL;
    mLockPiThreadId = 0;
    return OK;
}

status_t PlayerInterface::Pause()
{
    ALOGD("Pause mePlayingState:%d", mePlayingState);
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL || mePlayingState != E_MM_INTERFACE_STATE_PLAYING)
    {
        return INVALID_OPERATION;
    }
    status_t status = mMediaPlayer->pause();
    mePlayingState = E_MM_INTERFACE_STATE_PAUSE;
    mLockPiThreadId = 0;
    return status;
}

status_t PlayerInterface::Resume()
{
    ALOGD("Resume mePlayingState:%d", mePlayingState);
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL || mePlayingState != E_MM_INTERFACE_STATE_PAUSE)
    {
        return INVALID_OPERATION;
    }
    status_t status = mMediaPlayer->start();
    mePlayingState = E_MM_INTERFACE_STATE_PLAYING;
    mLockPiThreadId = 0;
    return status;
}

status_t PlayerInterface::SeekTo(int msec)
{
    ALOGD("SeekTo: %d\n", msec);
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    status_t sta =  mMediaPlayer->seekTo(msec);
    mLockPiThreadId = 0;
    return sta;

}

bool PlayerInterface::SetPlayMode(int speed)
{
    ALOGD("SetPlayMode:%d\n", speed);
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    bool bret =  mMediaPlayer->setPlayMode(speed);
    mLockPiThreadId = 0;
    return bret;
}

status_t PlayerInterface::setVolume(float leftVolume, float rightVolume)
{
    ALOGD("setVolume:L:%f R:%f\n", leftVolume,rightVolume);
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    status_t bret =  mMediaPlayer->setVolume(leftVolume,rightVolume);
    mLockPiThreadId = 0;
    return bret;
}
status_t PlayerInterface::getVideoWidth(int *w)
{
    ALOGD("getVideoWidth");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    status_t bret =  mMediaPlayer->getVideoWidth(w);
    mLockPiThreadId = 0;
    return bret;
}

status_t PlayerInterface::getVideoHeight(int *h)
{
    ALOGD("getVideoHeight\n");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    status_t bret =  mMediaPlayer->getVideoHeight(h);
    mLockPiThreadId = 0;
    return bret;
}
status_t PlayerInterface::getCurrentPosition(int *msec)
{
    ALOGD("getCurrentPosition\n");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    status_t bret =  mMediaPlayer->getCurrentPosition(msec);
    ALOGD("getCurrentPosition msec : %d  \n" ,*msec);
    mLockPiThreadId = 0;
    return bret;
}
status_t PlayerInterface::invoke(const Parcel& request, Parcel *reply)
{
    ALOGD("invoke\n");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    status_t bret =  mMediaPlayer->invoke(request,reply);
    mLockPiThreadId = 0;
    return bret;
}
status_t PlayerInterface::setAudioTrack(int track)
{
    ALOGD("setAudioTrack\n");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    status_t bret =  mMediaPlayer->setAudioTrack(track);
    mLockPiThreadId = 0;
    return bret;
}
status_t PlayerInterface::setSubtitleDataSource(const char *url)
{
    ALOGD("setSubtitleDataSource\n");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    status_t bret =  mMediaPlayer->setSubtitleDataSource(url);
    mLockPiThreadId = 0;
    return bret;
}
status_t PlayerInterface::setSubtitleSurface(const sp<IGraphicBufferProducer>& subSurfaceTexture)
{
    ALOGD("setSubtitleSurface\n");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    msubSurfaceTexture = subSurfaceTexture;
    status_t bret =  mMediaPlayer->setSubtitleSurface(subSurfaceTexture);
    mLockPiThreadId = 0;
    return bret;
}
void PlayerInterface::onSubtitleTrack()
{
    ALOGD("onSubtitleTrack\n");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return;
    }
    mMediaPlayer->onSubtitleTrack();
    return;
}
void PlayerInterface::offSubtitleTrack()
{
    ALOGD("offSubtitleTrack\n");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return;
    }
    mMediaPlayer->offSubtitleTrack();
    return;
}
status_t PlayerInterface::setSubtitleTrack(int track)
{
    ALOGD("setSubtitleTrack\n");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    status_t bret =  mMediaPlayer->setSubtitleTrack(track);
    mLockPiThreadId = 0;
    return bret;
}
status_t PlayerInterface::setParameter(int key, const Parcel& request)
{
    ALOGD("setParameter\n");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    status_t bret =  mMediaPlayer->setParameter(key,request);
    mLockPiThreadId = 0;
    return bret;
}

status_t PlayerInterface::getParameter(int key, Parcel *reply)
{
    ALOGD("getParameter\n");
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    status_t bret =  mMediaPlayer->getParameter(key,reply);
    mLockPiThreadId = 0;
    return bret;
}
status_t PlayerInterface::SetPlayModePermille(int speed_permille)
{
    ALOGD("SetPlayModePermille:%d\n", speed_permille);
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        return INVALID_OPERATION;
    }
    status_t bret = mMediaPlayer->setPlayModePermille(speed_permille);
    mLockPiThreadId = 0;
    return bret;
}

status_t PlayerInterface::SetOption(EN_MM_INTERFACE_OPTOIN_TYPE eOption, int arg1 )
{
    status_t status = OK;
    ST_MM_INTERFACE_TS_INFO *pstTsInfo = (ST_MM_INTERFACE_TS_INFO *)arg1;
    Parcel request;
    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(mMediaPlayer.get() == NULL)
    {
        ALOGD("SetOption fail error player state, %d, %d",(int)eOption, arg1);
        return INVALID_OPERATION;
    }
    ALOGD("SetOption , %d, %d",(int)eOption, arg1);
    switch (eOption) {
    case E_MM_INTERFACE_OPTION_SET_START_TIME:
        mStartTime = arg1;
        break;

    case E_MM_INTERFACE_OPTION_RESET_BUF:
        if (arg1 == true) {
            request.writeInt32(1);
        } else {
            request.writeInt32(0);
        }
        if (mePlayingState >= E_MM_INTERFACE_STATE_PLAYING) {
            mMediaPlayer->setParameter(KEY_PARAMETER_DEMUX_RESET, request);
        }
        break;

    case E_MM_INTERFACE_OPTION_ENABLE_SEAMLESS:
        if (arg1 == true) {
            request.writeInt32((int)E_MM_INTERFACE_SEAMLESS_DS_FREEZE);
        } else {
            request.writeInt32( (int)E_MM_INTERFACE_SEAMLESS_DS_BLANK);
        }
        status = mMediaPlayer->setParameter(KEY_PARAMETER_SET_SEAMLESS_MODE,  request);
        break;
    case E_MM_INTERFACE_OPTION_SEAMLESS_MODE:
        request.writeInt32(arg1);
        status = mMediaPlayer->setParameter(KEY_PARAMETER_SET_SEAMLESS_MODE,  request);
        break;
    case E_MM_INTERFACE_OPTION_CHANGE_PROGRAM:
        if (pstTsInfo != NULL) {
            request.writeInt32(pstTsInfo->u32ProgID);
            request.writeInt32((int)pstTsInfo->enVideoCodecType);
            request.writeInt32((int)pstTsInfo->enAudioCodecType);
            request.writeInt32(pstTsInfo->u32VideoPID);
            request.writeInt32(pstTsInfo->u32AudioPID);
            request.writeInt32(pstTsInfo->u32TransportPacketLen);
            status = mMediaPlayer->setParameter(KEY_PARAMETER_CHANGE_PROGRAM,  request);
        }
        break;

    case E_MM_INTERFACE_OPTION_SET_TS_INFO:
        if (pstTsInfo != NULL) {
            request.writeInt32(pstTsInfo->u32ProgID);
            request.writeInt32((int)pstTsInfo->enVideoCodecType);
            request.writeInt32((int)pstTsInfo->enAudioCodecType);
            request.writeInt32(pstTsInfo->u32VideoPID);
            request.writeInt32(pstTsInfo->u32AudioPID);
            request.writeInt32(pstTsInfo->u32TransportPacketLen);
            status = mMediaPlayer->setParameter(KEY_PARAMETER_SET_TS_INFO,  request);
        }
        break;

    case E_MM_INTERFACE_OPTION_SET_VIDEO_DECODE_ALL:
        mMediaPlayer->setParameter(KEY_PARAMETER_SET_VIDEO_DECODE_ALL, request);
        break;

    case E_MM_INTERFACE_OPTION_SET_VIDEO_DECODE_I_ONLY:
        mMediaPlayer->setParameter(KEY_PARAMETER_SET_VIDEO_DECODE_I_ONLY, request);
        break;

    case E_MM_INTERFACE_OPTION_SET_DROP_ERROR_FRAME:
		if (arg1 == 0) {
			mDropErrMode = 0;
            request.writeInt32(0);
        } else {
            mDropErrMode = 1;
            request.writeInt32(1);
        }
        if (mePlayingState >= E_MM_INTERFACE_STATE_INIT_DONE) {
            mMediaPlayer->setParameter(KEY_PARAMETER_SET_DROP_ERROR_FRAME, request);
        }
		break;

    default:
        break;

    }
    mLockPiThreadId = 0;
    return status;
}

int PlayerInterface::GetOption(EN_MM_INTERFACE_INFO_TYPE eInfo )
{
    int u32Ret = 0;
    Parcel reply;

    mLockPiThreadId = getThreadId();
    Mutex::Autolock autoLock(mLock);
    if(!mMediaPlayer.get())
        return INVALID_OPERATION;

    switch (eInfo) {
    case  E_MM_INTERFACE_INFO_TOTAL_TIME:
        mMediaPlayer->getDuration(&u32Ret);
        break;

    case E_MM_INTERFACE_INFO_CUR_TIME:
        mMediaPlayer->getCurrentPosition(&u32Ret);
        break;
    case E_MM_INTERFACE_INFO_ACCETPT_NEXT_SEAMLESS:
        mMediaPlayer->getParameter(KEY_PARAMETER_ACCEPT_NEXT_SEAMLESS,&reply);
        u32Ret = reply.readInt32();
        break;

    case E_MM_INTERFACE_INFO_BUFFER_PERCENT:
        u32Ret = mPercentRightNow;
        break;

    case E_MM_INTERFACE_INFO_GET_BUFFER_TOTAL_SIZE:
        mMediaPlayer->getParameter(KEY_PARAMETER_GET_BUFFER_STATUS,&reply);
        u32Ret = reply.readInt32();//free buffer
        u32Ret = reply.readInt32();//total buffer
        break;

    case E_MM_INTERFACE_INFO_GET_BUFFER_USED_SIZE:
        mMediaPlayer->getParameter(KEY_PARAMETER_GET_BUFFER_STATUS,&reply);
        u32Ret = reply.readInt32();//free buffer
        break;

    default:
        break;
    }
    mLockPiThreadId = 0;
    return u32Ret;
}

bool PlayerInterface::PushPayload()
{
    Parcel parcel;
    mMediaPlayer->setParameter(KEY_PARAMETER_PAYLOAD_SHOT,parcel);
    return true;
}

bool PlayerInterface::SwitchToPushPayloadMode()
{
    Parcel parcel;
    mMediaPlayer->setParameter(KEY_PARAMETER_SWITCH_TO_PUSH_DATA_MODE,parcel);
    return true;
}


status_t PlayerInterface::_StateProcess()
{
    Mutex::Autolock autoLock(mLock);

    if(mMyVector.size())
    {
        Vector<EN_MM_INTERFACE_PLAYING_STATE>::iterator it = mMyVector.begin();
        mePlayingState = *it;
        mMyVector.erase(it);
        switch (mePlayingState) {
        case E_MM_INTERFACE_STATE_INIT_DONE:
            ALOGD("_StateProcess,INIT_DONE pid = %p",getThreadId());
            if (mStartTime != -1) {
                mLock.unlock();
                SeekTo(mStartTime * 1000); // ms
                mStartTime = -1;
                mLock.lock();
            }
            if (mMediaPlayer.get()) {
                Parcel request;
                request.writeInt32(mDropErrMode);
                mMediaPlayer->setParameter(KEY_PARAMETER_SET_DROP_ERROR_FRAME, request);
            }

            if(mMediaPlayer.get())
            {
                mMediaPlayer->start();
            }
            mePlayingState = E_MM_INTERFACE_STATE_PLAYING;
            break;

        case E_MM_INTERFACE_STATE_PLAY_DONE:
            ALOGD("be about to call stop()\n");
            mePlayingState = E_MM_INTERFACE_STATE_NULL;
            break;

        case E_MM_INTERFACE_STATE_DATA_INSUFFICENT:
            ALOGD("E_MM_INTERFACE_STATE_DATA_INSUFFICENT\n");
            //pause();
            //mePlayingState = E_MM_INTERFACE_STATE_PAUSE;
            break;

        case E_MM_INTERFACE_STATE_DATA_SUFFICENT:
            ALOGD("E_MM_INTERFACE_STATE_DATA_SUFFICENT\n");
            // start();
            // mePlayingState = E_MM_INTERFACE_STATE_PLAYING;
            break;

        default:
            break;
        }
    }
    return OK;
}

status_t PlayerInterface::MonitorThread::readyToRun()
{
    return OK;
}

bool PlayerInterface::MonitorThread::threadLoop()
{
    ALOGD("Enter MonitorThread::threadLoop\n");
    while (!exitPending()) {
        mPlayer->_StateProcess();
        usleep(MM_INTERFACE_THREAD_SLEEP_MS);
    }
    ALOGD("Leave MonitorThread::threadLoop\n");

    return true;
}

};  // namespace android
