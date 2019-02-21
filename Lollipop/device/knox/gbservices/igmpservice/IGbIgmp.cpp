#define LOG_TAG "IGbIgmp"

#include <binder/Parcel.h>

#include <IGbIgmp.h>
#include "GbLog.h"

namespace android
{

enum
{
    OEPN = IBinder::FIRST_CALL_TRANSACTION,
    MEMBERSHIP,
    CLOSE,
    // DISCONNECT,
};

class BpGbIgmp: public BpInterface<IGbIgmp>
{
public:
    BpGbIgmp(const sp<IBinder>& impl)
        : BpInterface<IGbIgmp>(impl)
    {
    }

    void Open(const char* i_pLocalAddr, const char* i_pMulticastAddr)
    {
        // GLOGE(LOG_TAG, "Open [%s, %s]", i_pLocalAddr, i_pMulticastAddr);
        Parcel data, reply;
        data.writeInterfaceToken(IGbIgmp::getInterfaceDescriptor());
        data.writeCString(i_pLocalAddr);
        data.writeCString(i_pMulticastAddr);
        remote()->transact(OEPN, data, &reply);
        return;
    }

    void MemberShip(void)
    {
        // GLOGE(LOG_TAG, "MemberShip");
        Parcel data, reply;
        data.writeInterfaceToken(IGbIgmp::getInterfaceDescriptor());
        remote()->transact(MEMBERSHIP, data, &reply);
        return;
    }

    void Close(void)
    {
        // GLOGE(LOG_TAG, "Close");
        Parcel data, reply;
        data.writeInterfaceToken(IGbIgmp::getInterfaceDescriptor());
        remote()->transact(CLOSE, data, &reply);
        return;
    }

    // void disconnect()
    // {
    //     Parcel data, reply;
    //     data.writeInterfaceToken(IGbIgmp::getInterfaceDescriptor());
    //     remote()->transact(DISCONNECT, data, &reply);
    // }
};

IMPLEMENT_META_INTERFACE(GbIgmp, "gb.utils.IGbIgmp");

//-------------------------------------

status_t BnGbIgmp::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) 
    {
        case OEPN: {
            CHECK_INTERFACE(IGbIgmp, data, reply);
            // TODO
            // GLOGE(LOG_TAG, "BnGbIgmp OEPN");
            const char* i_pLocalAddr = data.readCString();
            const char* i_pMulticastAddr = data.readCString();
            // void GbIgmpService::Client::Open(const char* i_pLocalAddr, const char* i_pMulticastAddr)
            Open(i_pLocalAddr, i_pMulticastAddr);
            return NO_ERROR;
        } break;

        case MEMBERSHIP: {
            CHECK_INTERFACE(IGbIgmp, data, reply);
            // TODO
            // GLOGE(LOG_TAG, "BnGbIgmp MEMBERSHIP");
            // GbIgmpService::Client::MemberShip(void)
            MemberShip();
            return NO_ERROR;
        } break;

        case CLOSE: {
            CHECK_INTERFACE(IGbIgmp, data, reply);
            // TODO
            // GLOGE(LOG_TAG, "BnGbIgmp CLOSE");
            // GbIgmpService::Client::Close(void)
            Close();
            return NO_ERROR;
        } break;

        // case DISCONNECT: {
        //     CHECK_INTERFACE(IGbIgmp, data, reply);
        //     GLOGE(LOG_TAG, "BnGbIgmp DISCONNECT");
        //     disconnect();
        //     return NO_ERROR;
        // }

        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

}; // namespace android