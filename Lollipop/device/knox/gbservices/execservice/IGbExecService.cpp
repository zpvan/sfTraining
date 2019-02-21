#include <IGbExecService.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>

#include <utils/Errors.h>
#include <utils/String8.h>

namespace android {

enum {
    KILL = IBinder::FIRST_CALL_TRANSACTION,
};

class BpGbExecService: public BpInterface<IGbExecService>
{
public:
    BpGbExecService(const sp<IBinder>& impl)
        : BpInterface<IGbExecService>(impl)
    {
    }

    // TODO
    virtual void kill(const char *i_pid) {
        Parcel data, reply; 
        data.writeInterfaceToken(IGbExecService::getInterfaceDescriptor());
        data.writeCString(i_pid);
        remote()->transact(KILL, data, &reply);
        return;
    }
    
};

IMPLEMENT_META_INTERFACE(GbExecService, "gb.utils.IGbExecService");

//-------------------------------------

status_t BnGbExecService::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) 
    {
        case KILL: {
            CHECK_INTERFACE(IGbExecService, data, reply);
            // TODO
            const char *i_pid = data.readCString();
            kill(i_pid);
            return NO_ERROR;
        } break;

        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

}; // namespace android