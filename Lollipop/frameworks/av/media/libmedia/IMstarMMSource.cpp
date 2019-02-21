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

#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>

#include <utils/Errors.h>  // for status_t
#include <media/IMstarMMSource.h>

namespace android {

enum {
    INTT_CHECK = IBinder::FIRST_CALL_TRANSACTION,
    GET_DATASOURCE_BUFF,
    READ_FROM_OFFSET,
    READ_FROM_BEGIN,
    SEEK_AT,
    TELL_AT,
    GET_SIZE,
    GET_DATASOURCE_INFO,
    GET_ES_DATA,
};

class BpMstarMMSource: public BpInterface<IMstarMMSource> {
public:
    BpMstarMMSource(const sp<IBinder>& impl)
    : BpInterface<IMstarMMSource>(impl)
    {
    }

    virtual status_t initCheck()
    {
        //LOGD("IMstarMMSource::initCheck");
        Parcel data,reply;
        data.writeInterfaceToken(IMstarMMSource::getInterfaceDescriptor());
        remote()->transact(INTT_CHECK,data,&reply);
        return reply.readInt32();
    }

    virtual sp<IMemoryHeap> getDataSourceBuff()
    {
        //LOGD("IMstarMMSource::initCheck");
        Parcel data,reply;
        data.writeInterfaceToken(IMstarMMSource::getInterfaceDescriptor());
        remote()->transact(GET_DATASOURCE_BUFF,data,&reply);

        return interface_cast<IMemoryHeap>(reply.readStrongBinder());;
    }

    virtual ssize_t readAt(off_t offset, void *data, size_t size)
    {
        //LOGD("IMstarMMSource::readAt");
        //Remove unused parameter void *data , Because extContentSource use mDataSourceMemory instead of it.
        Parcel par_data,reply;
        par_data.writeInterfaceToken(IMstarMMSource::getInterfaceDescriptor());
        par_data.writeInt32(offset);
        par_data.writeInt32(size);

        remote()->transact(READ_FROM_OFFSET,par_data,&reply);
        return reply.readInt32();
    }

    virtual ssize_t readAt(void* data, size_t size)
    {
        //LOGD("IMstarMMSource::readAt");
        //Remove unused parameter void *data , Because extContentSource use mDataSourceMemory instead of it.
        Parcel par_data,reply;
        par_data.writeInterfaceToken(IMstarMMSource::getInterfaceDescriptor());
        par_data.writeInt32(size);

        remote()->transact(READ_FROM_BEGIN,par_data,&reply);
        return reply.readInt32();
    }

    virtual int seekAt(unsigned long long offset)
    {
        //LOGD("IMstarMMSource::seekAt");
        Parcel par_data,reply;
        par_data.writeInterfaceToken(IMstarMMSource::getInterfaceDescriptor());
        par_data.writeInt64(offset);

        remote()->transact(SEEK_AT,par_data,&reply);
        return reply.readInt32();
    }

    virtual unsigned long long tellAt()
    {
        //LOGD("IMstarMMSource::tellAt");
        Parcel par_data,reply;
        par_data.writeInterfaceToken(IMstarMMSource::getInterfaceDescriptor());

        remote()->transact(TELL_AT,par_data,&reply);
        return reply.readInt64();
    }

    virtual status_t getSize(unsigned long long *size)
    {
        //LOGD("IMstarMMSource::getSize");
        Parcel par_data,reply;
        par_data.writeInterfaceToken(IMstarMMSource::getInterfaceDescriptor());

        remote()->transact(GET_SIZE,par_data,&reply);
        *size = reply.readInt64();
        return reply.readInt32();
    }

    virtual status_t getDataSourceInfo(ST_MS_DATASOURCE_INFO &dataSourceInfo)
    {
        //LOGD("IMstarMMSource::getDataSourceInfo");
        Parcel par_data,reply;
        par_data.writeInterfaceToken(IMstarMMSource::getInterfaceDescriptor());
        remote()->transact(GET_DATASOURCE_INFO,par_data,&reply);
        dataSourceInfo.ePlayMode = (EN_MS_DATASOURCE_PLAYER_TYPE)reply.readInt32();
        dataSourceInfo.eContentType = (EN_MS_DATASOURCE_CONTENT_TYPE)reply.readInt32();
        dataSourceInfo.eESVideoCodec = (EN_MS_DATASOURCE_ES_VIDEO_CODEC)reply.readInt32();
        dataSourceInfo.eESAudioCodec = (EN_MS_DATASOURCE_ES_AUDIO_CODEC)reply.readInt32();
        dataSourceInfo.eMediaFormat = (EN_MS_DATASOURCE_MEDIA_FORMAT_TYPE)reply.readInt32();
        dataSourceInfo.eAppType= (EN_MS_DATASOURCE_AP_TYPE)reply.readInt32();
        dataSourceInfo.eAppMode= (EN_MS_DATASOURCE_APP_MODE)reply.readInt32();
        dataSourceInfo.ePlayerType = (EN_MS_PLAYER_TYPE)reply.readInt32();
        dataSourceInfo.u32TeeEncryptFlag = reply.readInt32();
        dataSourceInfo.bUseRingBuff = (bool)reply.readInt32();
        dataSourceInfo.bPC2TVMode = (bool)reply.readInt32();
        dataSourceInfo.bAutoPauseResume = (bool)reply.readInt32();
        return reply.readInt32();
    }
    virtual sp<IMemory> getESData()
    {
        //LOGD("IMstarMMSource::initCheck");
        Parcel data,reply;
        data.writeInterfaceToken(IMstarMMSource::getInterfaceDescriptor());
        remote()->transact(GET_ES_DATA,data,&reply);
        return interface_cast<IMemory>(reply.readStrongBinder());;
    }
};

IMPLEMENT_META_INTERFACE(MstarMMSource, "android.media.IMstarMMSource");

// ----------------------------------------------------------------------

status_t BnMstarMMSource::onTransact(
                                    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
    case INTT_CHECK: {
            CHECK_INTERFACE(IMstarMMSource, data, reply);

            status_t result = initCheck();
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
    case GET_DATASOURCE_BUFF:{
            CHECK_INTERFACE(IMstarMMSource, data, reply);
            sp<IMemoryHeap> buff = getDataSourceBuff();
            reply->writeStrongBinder(buff->asBinder());
            return NO_ERROR;
        }
    case READ_FROM_OFFSET:{
            CHECK_INTERFACE(IMstarMMSource, data, reply);
            off_t offset = data.readInt32();
            size_t size = data.readInt32();

            ssize_t result = readAt(offset, NULL, size);
            reply->writeInt32(result);
            return NO_ERROR;
        }break;
    case READ_FROM_BEGIN:{
            CHECK_INTERFACE(IMstarMMSource, data, reply);
            size_t size = data.readInt32();

            ssize_t result = readAt(NULL, size);
            reply->writeInt32(result);
            return NO_ERROR;
        }break;
    case SEEK_AT:{
            CHECK_INTERFACE(IMstarMMSource, data, reply);
            unsigned long long offset = data.readInt64();

            int result = seekAt(offset);
            reply->writeInt32(result);
            return NO_ERROR;
        }break;
    case TELL_AT:{
            CHECK_INTERFACE(IMstarMMSource, data, reply);

            unsigned long long result = tellAt();
            reply->writeInt64(result);
            return NO_ERROR;
        }break;
    case GET_SIZE:{
            CHECK_INTERFACE(IMstarMMSource, data, reply);

            unsigned long long size;
            status_t result = getSize(&size);
            reply->writeInt64(size);
            reply->writeInt32(result);
            return NO_ERROR;
        }break;
    case GET_DATASOURCE_INFO:{
            CHECK_INTERFACE(IMstarMMSource, data, reply);
            ST_MS_DATASOURCE_INFO dataSource_info;
            status_t result = getDataSourceInfo(dataSource_info);
            reply->writeInt32(dataSource_info.ePlayMode);
            reply->writeInt32(dataSource_info.eContentType);
            reply->writeInt32(dataSource_info.eESVideoCodec);
            reply->writeInt32(dataSource_info.eESAudioCodec);
            reply->writeInt32(dataSource_info.eMediaFormat);
            reply->writeInt32(dataSource_info.eAppType);
            reply->writeInt32(dataSource_info.eAppMode);
            reply->writeInt32(dataSource_info.ePlayerType);
            reply->writeInt32(dataSource_info.u32TeeEncryptFlag);
            reply->writeInt32(dataSource_info.bUseRingBuff);
            reply->writeInt32(dataSource_info.bPC2TVMode);
            reply->writeInt32(dataSource_info.bAutoPauseResume);
            reply->writeInt32(result);
            return NO_ERROR;
        }break;
    case GET_ES_DATA:{
            CHECK_INTERFACE(IMstarMMSource, data, reply);
            sp<IMemory> buff = getESData();
            reply->writeStrongBinder(buff->asBinder());
            return NO_ERROR;
        }
    default:
        return BBinder::onTransact(code, data, reply, flags);
    }
}

};  // namespace android
