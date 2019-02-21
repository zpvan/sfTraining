/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <arpa/inet.h>
#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>

#include <media/IMediaHTTPService.h>
#include <media/IMediaPlayer.h>
#include <media/IStreamSource.h>

#include <gui/IGraphicBufferProducer.h>
#include <utils/String8.h>

namespace android {

enum {
    DISCONNECT = IBinder::FIRST_CALL_TRANSACTION,
    SET_DATA_SOURCE_URL,
    SET_DATA_SOURCE_FD,
    SET_DATA_SOURCE_STREAM,
    // MStar Android Patch Begin
    SET_DATA_SOURCE_MSTARMM,
    // MStar Android Patch End
    PREPARE_ASYNC,
    START,
    STOP,
    IS_PLAYING,
    PAUSE,
    SEEK_TO,
    GET_CURRENT_POSITION,
    GET_DURATION,
    RESET,
    SET_AUDIO_STREAM_TYPE,
    SET_LOOPING,
    SET_VOLUME,
    INVOKE,
    SET_METADATA_FILTER,
    GET_METADATA,
    SET_AUX_EFFECT_SEND_LEVEL,
    ATTACH_AUX_EFFECT,
    SET_VIDEO_SURFACETEXTURE,
    SET_PARAMETER,
    GET_PARAMETER,
    SET_RETRANSMIT_ENDPOINT,
    GET_RETRANSMIT_ENDPOINT,
    SET_NEXT_PLAYER,
    // MStar Android Patch Begin
    SET_PLAYMODE,
    GET_PLAYMODE,
    SET_PLAYMODE_PERMILLE,
    GET_PLAYMODE_PERMILLE,
    SET_AUDIO_TRACK,
    SET_SUBTITLE_TRACK,
    SET_SUBTITLE_DATASOURCE,
    OFF_SUBTITLETRACK,
    ON_SUBTITLETRACK,
    GET_SUBTITLEDATA,
    GET_AUDIOTRACK_STRINGDATA,
    SET_SUBTITLE_SURFACE,
    SET_SUBTITLE_SYNC,
    DIVX_SET_TITLE,
    DIVX_GET_TITLE,
    DIVX_SET_EDITION,
    DIVX_GET_EDITION,
    DIVX_SET_CHAPTER,
    DIVX_GET_CHAPTER,
    DIVX_SET_AUTOCHAPTER,
    DIVX_GET_AUTOCHAPTER,
    DIVX_GET_AUTOCHAPTERTIME,
    DIVX_GET_CHAPTERTIME,
    DIVX_SET_RESUMEINFO,
    DIVX_SET_REPLAY_FLAG,
    CAPTURE_MOVIE_THUMBNAIL,
    SET_VIDEODISPLAYASPECTRATIO,
    GET_VIDEO_PTS,
    SET_AV_NOTCHANGED,
    // MStar Android Patch End
};

class BpMediaPlayer: public BpInterface<IMediaPlayer>
{
public:
    BpMediaPlayer(const sp<IBinder>& impl)
        : BpInterface<IMediaPlayer>(impl)
    {
    }

    // disconnect from media player service
    void disconnect()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(DISCONNECT, data, &reply);
    }

    status_t setDataSource(
            const sp<IMediaHTTPService> &httpService,
            const char* url,
            const KeyedVector<String8, String8>* headers)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(httpService != NULL);
        if (httpService != NULL) {
            data.writeStrongBinder(httpService->asBinder());
        }
        data.writeCString(url);
        if (headers == NULL) {
            data.writeInt32(0);
        } else {
            // serialize the headers
            data.writeInt32(headers->size());
            for (size_t i = 0; i < headers->size(); ++i) {
                data.writeString8(headers->keyAt(i));
                data.writeString8(headers->valueAt(i));
            }
        }
        remote()->transact(SET_DATA_SOURCE_URL, data, &reply);
        return reply.readInt32();
    }

    status_t setDataSource(int fd, int64_t offset, int64_t length) {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeFileDescriptor(fd);
        data.writeInt64(offset);
        data.writeInt64(length);
        remote()->transact(SET_DATA_SOURCE_FD, data, &reply);
        return reply.readInt32();
    }

    status_t setDataSource(const sp<IStreamSource> &source) {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeStrongBinder(source->asBinder());
        remote()->transact(SET_DATA_SOURCE_STREAM, data, &reply);
        return reply.readInt32();
    }

    // MStar Android Patch Begin
    status_t setDataSource(const sp<IMstarMMSource>&dataSource) {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeStrongBinder(dataSource->asBinder());
        remote()->transact(SET_DATA_SOURCE_MSTARMM, data, &reply);
        return reply.readInt32();
    }
    // MStar Android Patch End

    // pass the buffered IGraphicBufferProducer to the media player service
    status_t setVideoSurfaceTexture(const sp<IGraphicBufferProducer>& bufferProducer)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        sp<IBinder> b(bufferProducer->asBinder());
        data.writeStrongBinder(b);
        remote()->transact(SET_VIDEO_SURFACETEXTURE, data, &reply);
        return reply.readInt32();
    }

    status_t prepareAsync()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(PREPARE_ASYNC, data, &reply);
        return reply.readInt32();
    }

    status_t start()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(START, data, &reply);
        return reply.readInt32();
    }

    status_t stop()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(STOP, data, &reply);
        return reply.readInt32();
    }

    status_t isPlaying(bool* state)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(IS_PLAYING, data, &reply);
        *state = reply.readInt32();
        return reply.readInt32();
    }

    status_t pause()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(PAUSE, data, &reply);
        return reply.readInt32();
    }

    status_t seekTo(int msec)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(msec);
        remote()->transact(SEEK_TO, data, &reply);
        return reply.readInt32();
    }

    status_t getCurrentPosition(int* msec)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(GET_CURRENT_POSITION, data, &reply);
        *msec = reply.readInt32();
        return reply.readInt32();
    }

    status_t getDuration(int* msec)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(GET_DURATION, data, &reply);
        *msec = reply.readInt32();
        return reply.readInt32();
    }

    status_t reset()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(RESET, data, &reply);
        return reply.readInt32();
    }

    status_t setAudioStreamType(audio_stream_type_t stream)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32((int32_t) stream);
        remote()->transact(SET_AUDIO_STREAM_TYPE, data, &reply);
        return reply.readInt32();
    }

    status_t setLooping(int loop)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(loop);
        remote()->transact(SET_LOOPING, data, &reply);
        return reply.readInt32();
    }

    status_t setVolume(float leftVolume, float rightVolume)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeFloat(leftVolume);
        data.writeFloat(rightVolume);
        remote()->transact(SET_VOLUME, data, &reply);
        return reply.readInt32();
    }

    status_t invoke(const Parcel& request, Parcel *reply)
    {
        // Avoid doing any extra copy. The interface descriptor should
        // have been set by MediaPlayer.java.
        return remote()->transact(INVOKE, request, reply);
    }

    status_t setMetadataFilter(const Parcel& request)
    {
        Parcel reply;
        // Avoid doing any extra copy of the request. The interface
        // descriptor should have been set by MediaPlayer.java.
        remote()->transact(SET_METADATA_FILTER, request, &reply);
        return reply.readInt32();
    }

    status_t getMetadata(bool update_only, bool apply_filter, Parcel *reply)
    {
        Parcel request;
        request.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        // TODO: Burning 2 ints for 2 boolean. Should probably use flags in an int here.
        request.writeInt32(update_only);
        request.writeInt32(apply_filter);
        remote()->transact(GET_METADATA, request, reply);
        return reply->readInt32();
    }

    status_t setAuxEffectSendLevel(float level)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeFloat(level);
        remote()->transact(SET_AUX_EFFECT_SEND_LEVEL, data, &reply);
        return reply.readInt32();
    }

    status_t attachAuxEffect(int effectId)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(effectId);
        remote()->transact(ATTACH_AUX_EFFECT, data, &reply);
        return reply.readInt32();
    }

    status_t setParameter(int key, const Parcel& request)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(key);
        if (request.dataSize() > 0) {
            data.appendFrom(const_cast<Parcel *>(&request), 0, request.dataSize());
        }
        remote()->transact(SET_PARAMETER, data, &reply);
        return reply.readInt32();
    }

    status_t getParameter(int key, Parcel *reply)
    {
        Parcel data;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(key);
        return remote()->transact(GET_PARAMETER, data, reply);
    }

    status_t setRetransmitEndpoint(const struct sockaddr_in* endpoint)
    {
        Parcel data, reply;
        status_t err;

        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        if (NULL != endpoint) {
            data.writeInt32(sizeof(*endpoint));
            data.write(endpoint, sizeof(*endpoint));
        } else {
            data.writeInt32(0);
        }

        err = remote()->transact(SET_RETRANSMIT_ENDPOINT, data, &reply);
        if (OK != err) {
            return err;
        }
        return reply.readInt32();
    }

    status_t setNextPlayer(const sp<IMediaPlayer>& player) {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        sp<IBinder> b(player->asBinder());
        data.writeStrongBinder(b);
        remote()->transact(SET_NEXT_PLAYER, data, &reply);
        return reply.readInt32();
    }

    status_t getRetransmitEndpoint(struct sockaddr_in* endpoint)
    {
        Parcel data, reply;
        status_t err;

        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        err = remote()->transact(GET_RETRANSMIT_ENDPOINT, data, &reply);

        if ((OK != err) || (OK != (err = reply.readInt32()))) {
            return err;
        }

        data.read(endpoint, sizeof(*endpoint));

        return err;
    }
    // MStar Android Patch Begin
    bool setPlayMode(int speed)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(speed);
        remote()->transact(SET_PLAYMODE, data, &reply);
        return reply.readInt32();
    }

    int getPlayMode()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(GET_PLAYMODE, data, &reply);
        return reply.readInt32();
    }

    status_t setPlayModePermille(int speed_permille)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(speed_permille);
        remote()->transact(SET_PLAYMODE_PERMILLE, data, &reply);
        return reply.readInt32();
    }

    int getPlayModePermille()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(GET_PLAYMODE_PERMILLE, data, &reply);
        return reply.readInt32();
    }

    status_t setAudioTrack(int track)
    {
        Parcel data,reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(track);
        remote()->transact(SET_AUDIO_TRACK,data,&reply);
        return reply.readInt32();
    }

    status_t setSubtitleTrack(int track)
    {
        Parcel data,reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(track);
        remote()->transact(SET_SUBTITLE_TRACK,data,&reply);
        return reply.readInt32();
    }

    status_t setSubtitleDataSource(const char *url)
    {
        Parcel data,reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        if (url)
            data.writeCString(url);
        else
            data.writeCString("");

        remote()->transact(SET_SUBTITLE_DATASOURCE,data,&reply);
        return reply.readInt32();
    }

    status_t offSubtitleTrack()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(OFF_SUBTITLETRACK, data, &reply);
        return reply.readInt32();
    }

    status_t onSubtitleTrack()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(ON_SUBTITLETRACK, data, &reply);
        return reply.readInt32();
    }

    char* getSubtitleData(int *size)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(GET_SUBTITLEDATA, data, &reply);
        *size = reply.readInt32();
        char *pStr = NULL;
        pStr = (char *)malloc(sizeof(char) * (*size));
        reply.read(pStr, sizeof(char) * (*size));
        return pStr;
    }

    char *getAudioTrackStringData(int *size, int infoType)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(infoType);
        remote()->transact(GET_AUDIOTRACK_STRINGDATA, data, &reply);
        *size = reply.readInt32();
        char *pStr = NULL;
        pStr = (char *)malloc(sizeof(char) * (*size));
        reply.read(pStr, sizeof(char) * (*size));
        return pStr;
    }

    status_t setSubtitleSurface(const sp<IGraphicBufferProducer>& surfaceTexture)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        sp<IBinder> b(surfaceTexture->asBinder());
        data.writeStrongBinder(b);
        remote()->transact(SET_SUBTITLE_SURFACE, data, &reply);
        return reply.readInt32();
    }

    int setSubtitleSync(int timeMs)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(timeMs);
        remote()->transact(SET_SUBTITLE_SYNC, data, &reply);
        return reply.readInt32();
    }

    void divx_SetTitle(uint32_t u32ID)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(u32ID);
        remote()->transact(DIVX_SET_TITLE, data, &reply);
        return;
    }

    status_t divx_GetTitle(uint32_t *pu32ID)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(DIVX_GET_TITLE, data, &reply);
        *pu32ID = reply.readInt32();
        return reply.readInt32();
    }

    void divx_SetEdition(uint32_t u32ID)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(u32ID);
        remote()->transact(DIVX_SET_EDITION, data, &reply);
        return;
    }

    status_t divx_GetEdition(uint32_t *pu32ID)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(DIVX_GET_EDITION, data, &reply);
        *pu32ID = reply.readInt32();
        return reply.readInt32();
    }

    void divx_SetChapter(uint32_t u32ID)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(u32ID);
        remote()->transact(DIVX_SET_CHAPTER, data, &reply);
        return;
    }

    status_t divx_GetChapter(uint32_t *pu32ID)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(DIVX_GET_CHAPTER, data, &reply);
        *pu32ID = reply.readInt32();
        return reply.readInt32();
    }

    void divx_SetAutochapter(uint32_t u32ID)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(u32ID);
        remote()->transact(DIVX_SET_AUTOCHAPTER, data, &reply);
        return;
    }

    status_t divx_GetAutochapter(uint32_t *pu32ID)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(DIVX_GET_AUTOCHAPTER, data, &reply);
        *pu32ID = reply.readInt32();
        return reply.readInt32();
    }

    status_t divx_GetAutochapterTime(uint32_t u32ID, uint32_t *pu32IdTime)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(u32ID);
        remote()->transact(DIVX_GET_AUTOCHAPTERTIME, data, &reply);
        *pu32IdTime = reply.readInt32();
        return reply.readInt32();
    }

    status_t divx_GetChapterTime(uint32_t u32ID, uint32_t *pu32IdTime)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(u32ID);
        remote()->transact(DIVX_GET_CHAPTERTIME, data, &reply);
        *pu32IdTime = reply.readInt32();
        return reply.readInt32();
    }

    status_t divx_SetResumePlay(const Parcel &inResumeInfo)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        if (inResumeInfo.dataSize() > 0) {
            data.appendFrom(const_cast<Parcel *>(&inResumeInfo), 0, inResumeInfo.dataSize());
        }
        remote()->transact(DIVX_SET_RESUMEINFO, data, &reply);
        return reply.readInt32();
    }

    status_t divx_SetReplayFlag(bool isResumePlay)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(isResumePlay);
        remote()->transact(DIVX_SET_REPLAY_FLAG, data, &reply);
        return reply.readInt32();
    }

    status_t captureMovieThumbnail(sp<IMemory>& mem, Parcel *pstCaptureParam, int* pCapBufPitch)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeStrongBinder(mem->asBinder());
        /*
        remote()->transact(CAPTURE_MOVIE_THUMBNAIL,data,pstCaptureParam);
        *pCapBufPitch = pstCaptureParam->readInt32();
        return pstCaptureParam->readInt32();
        */
        if (pstCaptureParam->dataSize() > 0) {
            data.appendFrom(const_cast<Parcel *>(pstCaptureParam), 0, pstCaptureParam->dataSize());
        }
	//LOGV("Send CaptureMovieThumbnail\n");
        remote()->transact(CAPTURE_MOVIE_THUMBNAIL,data,&reply);
		*pCapBufPitch = reply.readInt32();
        return reply.readInt32();
    }

    status_t setVideoDisplayAspectRatio(int eRatio)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(eRatio);
        remote()->transact(SET_VIDEODISPLAYASPECTRATIO, data, &reply);
        return reply.readInt32();
    }

    status_t getVideoPTS(int* msec)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(GET_VIDEO_PTS, data, &reply);
        *msec = reply.readInt32();
        return reply.readInt32();
    }

    status_t setAVCodecChanged(bool changed)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(changed);
        remote()->transact(SET_AV_NOTCHANGED, data, &reply);
        return reply.readInt32();
    }
    // MStar Android Patch End
};

IMPLEMENT_META_INTERFACE(MediaPlayer, "android.media.IMediaPlayer");

// ----------------------------------------------------------------------

status_t BnMediaPlayer::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case DISCONNECT: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            disconnect();
            return NO_ERROR;
        } break;
        case SET_DATA_SOURCE_URL: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);

            sp<IMediaHTTPService> httpService;
            if (data.readInt32()) {
                httpService =
                    interface_cast<IMediaHTTPService>(data.readStrongBinder());
            }

            const char* url = data.readCString();
            KeyedVector<String8, String8> headers;
            int32_t numHeaders = data.readInt32();
            for (int i = 0; i < numHeaders; ++i) {
                String8 key = data.readString8();
                String8 value = data.readString8();
                headers.add(key, value);
            }
            reply->writeInt32(setDataSource(
                        httpService, url, numHeaders > 0 ? &headers : NULL));
            return NO_ERROR;
        } break;
        case SET_DATA_SOURCE_FD: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int fd = data.readFileDescriptor();
            int64_t offset = data.readInt64();
            int64_t length = data.readInt64();
            reply->writeInt32(setDataSource(fd, offset, length));
            return NO_ERROR;
        }
        case SET_DATA_SOURCE_STREAM: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            sp<IStreamSource> source =
                interface_cast<IStreamSource>(data.readStrongBinder());
            reply->writeInt32(setDataSource(source));
            return NO_ERROR;
        }
        // MStar Android Patch Begin
        case SET_DATA_SOURCE_MSTARMM: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            sp<IMstarMMSource> dataSource = interface_cast<IMstarMMSource>(data.readStrongBinder());
            reply->writeInt32(setDataSource(dataSource));
            return NO_ERROR;
        } break;
        // MStar Android Patch End
        case SET_VIDEO_SURFACETEXTURE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            sp<IGraphicBufferProducer> bufferProducer =
                    interface_cast<IGraphicBufferProducer>(data.readStrongBinder());
            reply->writeInt32(setVideoSurfaceTexture(bufferProducer));
            return NO_ERROR;
        } break;
        case PREPARE_ASYNC: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(prepareAsync());
            return NO_ERROR;
        } break;
        case START: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(start());
            return NO_ERROR;
        } break;
        case STOP: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(stop());
            return NO_ERROR;
        } break;
        case IS_PLAYING: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            bool state;
            status_t ret = isPlaying(&state);
            reply->writeInt32(state);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case PAUSE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(pause());
            return NO_ERROR;
        } break;
        case SEEK_TO: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(seekTo(data.readInt32()));
            return NO_ERROR;
        } break;
        case GET_CURRENT_POSITION: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int msec = 0;
            status_t ret = getCurrentPosition(&msec);
            reply->writeInt32(msec);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case GET_DURATION: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int msec = 0;
            status_t ret = getDuration(&msec);
            reply->writeInt32(msec);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case RESET: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(reset());
            return NO_ERROR;
        } break;
        case SET_AUDIO_STREAM_TYPE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setAudioStreamType((audio_stream_type_t) data.readInt32()));
            return NO_ERROR;
        } break;
        case SET_LOOPING: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setLooping(data.readInt32()));
            return NO_ERROR;
        } break;
        case SET_VOLUME: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            float leftVolume = data.readFloat();
            float rightVolume = data.readFloat();
            reply->writeInt32(setVolume(leftVolume, rightVolume));
            return NO_ERROR;
        } break;
        case INVOKE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            status_t result = invoke(data, reply);
            return result;
        } break;
        case SET_METADATA_FILTER: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setMetadataFilter(data));
            return NO_ERROR;
        } break;
        case GET_METADATA: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            bool update_only = static_cast<bool>(data.readInt32());
            bool apply_filter = static_cast<bool>(data.readInt32());
            const status_t retcode = getMetadata(update_only, apply_filter, reply);
            reply->setDataPosition(0);
            reply->writeInt32(retcode);
            reply->setDataPosition(0);
            return NO_ERROR;
        } break;
        case SET_AUX_EFFECT_SEND_LEVEL: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setAuxEffectSendLevel(data.readFloat()));
            return NO_ERROR;
        } break;
        case ATTACH_AUX_EFFECT: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(attachAuxEffect(data.readInt32()));
            return NO_ERROR;
        } break;
        case SET_PARAMETER: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int key = data.readInt32();

            Parcel request;
            if (data.dataAvail() > 0) {
                request.appendFrom(
                        const_cast<Parcel *>(&data), data.dataPosition(), data.dataAvail());
            }
            request.setDataPosition(0);
            reply->writeInt32(setParameter(key, request));
            return NO_ERROR;
        } break;
        case GET_PARAMETER: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            return getParameter(data.readInt32(), reply);
        } break;
        case SET_RETRANSMIT_ENDPOINT: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);

            struct sockaddr_in endpoint;
            memset(&endpoint, 0, sizeof(endpoint));
            int amt = data.readInt32();
            if (amt == sizeof(endpoint)) {
                data.read(&endpoint, sizeof(struct sockaddr_in));
                reply->writeInt32(setRetransmitEndpoint(&endpoint));
            } else {
                reply->writeInt32(setRetransmitEndpoint(NULL));
            }

            return NO_ERROR;
        } break;
        case GET_RETRANSMIT_ENDPOINT: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);

            struct sockaddr_in endpoint;
            memset(&endpoint, 0, sizeof(endpoint));
            status_t res = getRetransmitEndpoint(&endpoint);

            reply->writeInt32(res);
            reply->write(&endpoint, sizeof(endpoint));

            return NO_ERROR;
        } break;
        case SET_NEXT_PLAYER: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setNextPlayer(interface_cast<IMediaPlayer>(data.readStrongBinder())));

            return NO_ERROR;
        } break;
        // MStar Android Patch Begin
        case SET_PLAYMODE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setPlayMode(data.readInt32()));
            return NO_ERROR;
        } break;
        case GET_PLAYMODE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(getPlayMode());
            return NO_ERROR;
        } break;
        case SET_PLAYMODE_PERMILLE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setPlayModePermille(data.readInt32()));
            return NO_ERROR;
        } break;
        case GET_PLAYMODE_PERMILLE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(getPlayModePermille());
            return NO_ERROR;
        } break;
        case SET_AUDIO_TRACK: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setAudioTrack(data.readInt32()));
            return NO_ERROR;
        } break;
        case SET_SUBTITLE_TRACK: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setSubtitleTrack(data.readInt32()));
            return NO_ERROR;
        } break;
        case SET_SUBTITLE_DATASOURCE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setSubtitleDataSource(data.readCString()));
            return NO_ERROR;
        } break;
        case OFF_SUBTITLETRACK: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(offSubtitleTrack());
            return NO_ERROR;
        } break;
        case ON_SUBTITLETRACK:{
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(onSubtitleTrack());
            return NO_ERROR;
        } break;
        case GET_SUBTITLEDATA: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int size = 0;
            char *pStr = getSubtitleData(&size);
            reply->writeInt32(size);
            reply->write(pStr, sizeof(char) * size);
            //maybe need to free memory!!!!!!!!!!!
            //if(pStr != NULL)
            //    free(pStr);
            return NO_ERROR;
        } break;
        case GET_AUDIOTRACK_STRINGDATA: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int size = 0;
            char *pStr = getAudioTrackStringData(&size,data.readInt32());
            reply->writeInt32(size);
            reply->write(pStr, sizeof(char) * size);
            return NO_ERROR;
        } break;
        case SET_SUBTITLE_SURFACE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            sp<IGraphicBufferProducer> surfaceTexture = interface_cast<IGraphicBufferProducer>(data.readStrongBinder());
            reply->writeInt32(setSubtitleSurface(surfaceTexture));
            return NO_ERROR;
        } break;
        case SET_SUBTITLE_SYNC: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setSubtitleSync(data.readInt32()));
            return NO_ERROR;
        } break;
        case DIVX_SET_TITLE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            divx_SetTitle(data.readInt32());
            return NO_ERROR;
        } break;
        case DIVX_GET_TITLE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            uint32_t divx_title;
            status_t ret;
            ret = divx_GetTitle(&divx_title);
            reply->writeInt32(divx_title);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case DIVX_SET_EDITION: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            divx_SetEdition(data.readInt32());
            return NO_ERROR;
        } break;
        case DIVX_GET_EDITION: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            uint32_t divx_edition;
            status_t ret;
            ret = divx_GetEdition(&divx_edition);
            reply->writeInt32(divx_edition);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case DIVX_SET_CHAPTER: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            divx_SetChapter(data.readInt32());
            return NO_ERROR;
        } break;
        case DIVX_GET_CHAPTER: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            uint32_t divx_chapter;
            status_t ret;
            ret = divx_GetChapter(&divx_chapter);
            reply->writeInt32(divx_chapter);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case DIVX_SET_AUTOCHAPTER: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            divx_SetAutochapter(data.readInt32());
            return NO_ERROR;
        } break;
        case DIVX_GET_AUTOCHAPTER: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            uint32_t divx_autoChapter;
            status_t ret;
            ret = divx_GetAutochapter(&divx_autoChapter);
            reply->writeInt32(divx_autoChapter);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case DIVX_GET_AUTOCHAPTERTIME: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            uint32_t divx_autoChapterTime;
            status_t ret;
            ret = divx_GetAutochapterTime(data.readInt32(), &divx_autoChapterTime);
            reply->writeInt32(divx_autoChapterTime);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case DIVX_GET_CHAPTERTIME: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            uint32_t divx_chapterTime;
            status_t ret;
            ret = divx_GetChapterTime(data.readInt32(), &divx_chapterTime);
            reply->writeInt32(divx_chapterTime);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case DIVX_SET_RESUMEINFO: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(divx_SetResumePlay(data));
            return NO_ERROR;
        } break;
        case DIVX_SET_REPLAY_FLAG: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(divx_SetReplayFlag(data.readInt32()));
            return NO_ERROR;
        } break;
        case CAPTURE_MOVIE_THUMBNAIL:{
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            sp<IMemory> mem = interface_cast<IMemory> (data.readStrongBinder());
            int pitch = 0;
            // status_t ret = captureMovieThumbnail(mem,reply,&pitch);
            //  LOGV("ontransact CaptureMovieThumbnail\n");
            status_t ret = captureMovieThumbnail(mem,(Parcel *)&data,&pitch);
            reply->setDataPosition(0);
            reply->writeInt32(pitch);
            reply->writeInt32(ret);
            reply->setDataPosition(0);
            return NO_ERROR;
        } break;
        case SET_VIDEODISPLAYASPECTRATIO: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setVideoDisplayAspectRatio(data.readInt32()));
            return NO_ERROR;
        } break;
        case GET_VIDEO_PTS: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int msec;
            status_t ret = getVideoPTS(&msec);
            reply->writeInt32(msec);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case SET_AV_NOTCHANGED:{
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            status_t ret = setAVCodecChanged(data.readInt32());
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        // MStar Android Patch End
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android
