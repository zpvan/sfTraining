

#include <IGbIgmpService.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>

#include <utils/Errors.h>
#include <utils/String8.h>

namespace android {

enum {
    CREATE = IBinder::FIRST_CALL_TRANSACTION,
};

class BpGbIgmpService: public BpInterface<IGbIgmpService>
{
public:
    BpGbIgmpService(const sp<IBinder>& impl)
        : BpInterface<IGbIgmpService>(impl)
    {
    }

    // TODO
    virtual sp<IGbIgmp> create() {
        Parcel data, reply; 
        data.writeInterfaceToken(IGbIgmpService::getInterfaceDescriptor());
        remote()->transact(CREATE, data, &reply);
        return interface_cast<IGbIgmp>(reply.readStrongBinder());
    }
    
};

IMPLEMENT_META_INTERFACE(GbIgmpService, "gb.utils.IGbIgmpService");

//-------------------------------------

status_t BnGbIgmpService::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) 
    {
        case CREATE: {
            CHECK_INTERFACE(IGbIgmpService, data, reply);
            // TODO
            sp<IGbIgmp> gbIgmp = create();
            reply->writeStrongBinder(gbIgmp->asBinder());
            return NO_ERROR;
        } break;

        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

} // namespace android