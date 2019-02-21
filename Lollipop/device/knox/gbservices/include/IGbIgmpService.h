#ifndef I_GB_IGMP_SERVICE_H
#define I_GB_IGMP_SERVICE_H

#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include <IGbIgmp.h>

namespace android {

class IGbIgmpService: public IInterface
{
public:
    DECLARE_META_INTERFACE(GbIgmpService);

    virtual sp<IGbIgmp> create() = 0;
};

// ----------------------------------------------------------------------------

class BnGbIgmpService: public BnInterface<IGbIgmpService>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; // namespace android

#endif // I_GB_IGMP_SERVICE_H