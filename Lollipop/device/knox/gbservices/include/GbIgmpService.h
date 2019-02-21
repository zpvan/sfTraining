#ifndef GB_IGMP_SERVICE_H
#define GB_IGMP_SERVICE_H

#include <IGbIgmp.h>
#include <IGbIgmpService.h>
#include "igmp.h"
#include <utils/KeyedVector.h>
#include <utils/threads.h>


namespace android {

class GbIgmpService : public BnGbIgmpService
{
    class Client;

public:
    static void instantiate();

    // IGbIgmpService interface
    // TODO
    virtual sp<IGbIgmp> create();

    // TODO
    void removeClient(wp<Client> client);

private:
    /**
     * GbIgmpService::Client是gbIgmp的真正实例
     */
    class Client: public BnGbIgmp
    {
    public:
        Client(const sp<GbIgmpService>& service);
        ~Client();

        // virtual void disconnect();

        virtual void Open(const char* i_pLocalAddr, const char* i_pMulticastAddr);
        virtual void MemberShip(void);
        virtual void Close(void);
    private:
        CIGMP *mCigmp;
        sp<GbIgmpService>      mService;
    }; // Client

                            GbIgmpService();
    virtual                 ~GbIgmpService();

    mutable Mutex mLock;
    SortedVector< wp<Client> >  mClients;
    int32_t                     mNextConnId;
};

}; // namespace android

#endif // GB_IGMP_SERVICE_H