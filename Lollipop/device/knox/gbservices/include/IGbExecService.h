#ifndef I_GB_EXEC_SERVICE_H
#define I_GB_EXEC_SERVICE_H

#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

class IGbExecService: public IInterface
{
public:
    DECLARE_META_INTERFACE(GbExecService);

    virtual void kill(const char* i_pid) = 0;
};

// ----------------------------------------------------------------------------

class BnGbExecService: public BnInterface<IGbExecService>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; // namespace android

#endif // I_GB_EXEC_SERVICE_H