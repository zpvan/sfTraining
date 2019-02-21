#define LOG_TAG "GbIgmp"

#include "GbIgmp.h"
#include "GbLog.h"
#include <binder/IServiceManager.h>

namespace android {

Mutex IGbIgmpDeathNotifier::sServiceLock;
sp<IGbIgmpService> IGbIgmpDeathNotifier::sGbIgmpService;
sp<IGbIgmpDeathNotifier::DeathNotifier> IGbIgmpDeathNotifier::sDeathNotifier;
SortedVector< wp<IGbIgmpDeathNotifier> > IGbIgmpDeathNotifier::sObitRecipients;

//---------------------------------------------------------------------------

/*static*/ void
IGbIgmpDeathNotifier::addObitRecipient(const wp<IGbIgmpDeathNotifier>& recipient)
{
    // GLOGE(LOG_TAG, "addObitRecipient");
    Mutex::Autolock _l(sServiceLock);
    sObitRecipients.add(recipient);
    // GLOGE(LOG_TAG, "addObitRecipient done");
}

/*static*/ void
IGbIgmpDeathNotifier::removeObitRecipient(const wp<IGbIgmpDeathNotifier>& recipient)
{
    // GLOGE(LOG_TAG, "removeObitRecipient");
    Mutex::Autolock _l(sServiceLock);
    sObitRecipients.remove(recipient);
    // GLOGE(LOG_TAG, "removeObitRecipient done");
}

void
IGbIgmpDeathNotifier::DeathNotifier::binderDied(const wp<IBinder>& who __unused) {
    GLOGE(LOG_TAG, "gb gimp server died");

    // Need to do this with the lock held
    SortedVector< wp<IGbIgmpDeathNotifier> > list;
    {
        Mutex::Autolock _l(sServiceLock);
        sGbIgmpService.clear();
        list = sObitRecipients;
    }

    // Notify application when media server dies.
    // Don't hold the static lock during callback in case app
    // makes a call that needs the lock.
    size_t count = list.size();
    for (size_t iter = 0; iter < count; ++iter) {
        sp<IGbIgmpDeathNotifier> notifier = list[iter].promote();
        if (notifier != 0) {
            notifier->died();
        }
    }
}

IGbIgmpDeathNotifier::DeathNotifier::~DeathNotifier()
{
    // GLOGE(LOG_TAG, "DeathNotifier::~DeathNotifier");
    Mutex::Autolock _l(sServiceLock);
    sObitRecipients.clear();
    if (sGbIgmpService != 0) {
        sGbIgmpService->asBinder()->unlinkToDeath(this);
    }
}

static int GET_SERVER_LOOP = 2;

/*static*/
const sp<IGbIgmpService>& IGbIgmpDeathNotifier::getGbIgmpService()
{
    // GLOGE(LOG_TAG, "getGbIgmpService");
    Mutex::Autolock _l(sServiceLock);
    if (sGbIgmpService == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        int loop = GET_SERVER_LOOP;
        do {
            loop--;
            /**
             * if server manager can't find "gb.igmp.server", it will spend 5s
             */
            binder = sm->getService(String16("gb.igmp.server"));
            if (binder != 0) {
                break;
            }
            GLOGE(LOG_TAG, "gb igmp service not published, waiting...");
            usleep(100*1000); // 100ms
        } while (loop >= 0);
        if (binder != 0)
        {
            if (sDeathNotifier == NULL) {
                sDeathNotifier = new DeathNotifier();
            }
            binder->linkToDeath(sDeathNotifier);
            // GLOGE(LOG_TAG, "interface_cast begin");
            sGbIgmpService = interface_cast<IGbIgmpService>(binder);
            // GLOGE(LOG_TAG, "interface_cast end");
        } 
    }
    if (sGbIgmpService == 0)
    {
        GLOGE(LOG_TAG, "no gb igmp service!?");
    }
    return sGbIgmpService;
}

//-------------------------------------------------------------------------------

void GbIgmp::died()
{
    GLOGE(LOG_TAG, "died");
}

GbIgmp::GbIgmp()
{
    const sp<IGbIgmpService>& service = IGbIgmpDeathNotifier::getGbIgmpService();
    if (service != 0) {
        GLOGE(LOG_TAG, "getGbIgmpService successed!");
        sp<IGbIgmp> igmp(service->create());
        if (igmp == NULL)
        {
            GLOGE(LOG_TAG, "service->create() failed!");
            return;
        }
        mIgmp = igmp;
        return;
    }
    GLOGE(LOG_TAG, "getGbIgmpService failed!");
}

GbIgmp::~GbIgmp()
{
    // GLOGE(LOG_TAG, "~GbIgmp In");
    // disconnect();
    // GLOGE(LOG_TAG, "~GbIgmp Out");
}

// void GbIgmp::disconnect()
// {
//     GLOGE(LOG_TAG, "GbIgmp::disconnect() In");
//     sp<IGbIgmp> p;
//     {
//         // Mutex::Autolock _l(mLock);
//         p = mIgmp;
//         mIgmp.clear();
//     }

//     if (p != 0) {
//         p->disconnect();
//         p = 0;
//     }
//     GLOGE(LOG_TAG, "GbIgmp::disconnect() Out");
// }

void GbIgmp::Open(const char* i_pLocalAddr, const char* i_pMulticastAddr)
{
    if (mIgmp == NULL)
    {
        GLOGE(LOG_TAG, "mIgmp is NULL");
        return;
    }
    mIgmp->Open(i_pLocalAddr, i_pMulticastAddr);
}

void GbIgmp::MemberShip(void)
{
    if (mIgmp == NULL)
    {
        GLOGE(LOG_TAG, "mIgmp is NULL");
        return;
    }
    mIgmp->MemberShip();
}

void GbIgmp::Close(void)
{
    if (mIgmp == NULL)
    {
        GLOGE(LOG_TAG, "mIgmp is NULL");
        return;
    }
    mIgmp->Close();
}

}; // namespace android