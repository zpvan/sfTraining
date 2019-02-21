#define LOG_TAG "GbExec"

#include "GbExec.h"
#include "GbLog.h"
#include <binder/IServiceManager.h>

namespace android {

Mutex IGbExecDeathNotifier::sServiceLock;
sp<IGbExecService> IGbExecDeathNotifier::sGbExecService;
sp<IGbExecDeathNotifier::DeathNotifier> IGbExecDeathNotifier::sDeathNotifier;
SortedVector< wp<IGbExecDeathNotifier> > IGbExecDeathNotifier::sObitRecipients;

//---------------------------------------------------------------------------

/*static*/ void
IGbExecDeathNotifier::addObitRecipient(const wp<IGbExecDeathNotifier>& recipient)
{
    // GLOGE(LOG_TAG, "addObitRecipient");
    Mutex::Autolock _l(sServiceLock);
    sObitRecipients.add(recipient);
    // GLOGE(LOG_TAG, "addObitRecipient done");
}

/*static*/ void
IGbExecDeathNotifier::removeObitRecipient(const wp<IGbExecDeathNotifier>& recipient)
{
    // GLOGE(LOG_TAG, "removeObitRecipient");
    Mutex::Autolock _l(sServiceLock);
    sObitRecipients.remove(recipient);
    // GLOGE(LOG_TAG, "removeObitRecipient done");
}

void
IGbExecDeathNotifier::DeathNotifier::binderDied(const wp<IBinder>& who __unused) {
    GLOGE(LOG_TAG, "gb exec server died");

    // Need to do this with the lock held
    SortedVector< wp<IGbExecDeathNotifier> > list;
    {
        Mutex::Autolock _l(sServiceLock);
        sGbExecService.clear();
        list = sObitRecipients;
    }

    // Notify application when media server dies.
    // Don't hold the static lock during callback in case app
    // makes a call that needs the lock.
    size_t count = list.size();
    for (size_t iter = 0; iter < count; ++iter) {
        sp<IGbExecDeathNotifier> notifier = list[iter].promote();
        if (notifier != 0) {
            notifier->died();
        }
    }
}

IGbExecDeathNotifier::DeathNotifier::~DeathNotifier()
{
    // GLOGE(LOG_TAG, "DeathNotifier::~DeathNotifier");
    Mutex::Autolock _l(sServiceLock);
    sObitRecipients.clear();
    if (sGbExecService != 0) {
        sGbExecService->asBinder()->unlinkToDeath(this);
    }
}

static int GET_SERVER_LOOP = 2;

/*static*/
const sp<IGbExecService>& IGbExecDeathNotifier::getGbExecService()
{
    // GLOGE(LOG_TAG, "getGbExecService");
    Mutex::Autolock _l(sServiceLock);
    if (sGbExecService == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        int loop = GET_SERVER_LOOP;
        do {
            loop--;
            /**
             * if server manager can't find "gb.exec.server", it will spend 5s
             */
            binder = sm->getService(String16("gb.exec.server"));
            if (binder != 0) {
                break;
            }
            GLOGE(LOG_TAG, "gb exec service not published, waiting...");
            usleep(100*1000); // 100ms
        } while (loop >= 0);
        if (binder != 0)
        {
            if (sDeathNotifier == NULL) {
                sDeathNotifier = new DeathNotifier();
            }
            binder->linkToDeath(sDeathNotifier);
            GLOGE(LOG_TAG, "interface_cast begin");
            sGbExecService = interface_cast<IGbExecService>(binder);
            GLOGE(LOG_TAG, "interface_cast end");
        } 
    }
    if (sGbExecService == 0)
    {
        GLOGE(LOG_TAG, "no gb exec service!?");
    }
    return sGbExecService;
}

//-------------------------------------------------------------------------------

void GbExec::died()
{
    GLOGE(LOG_TAG, "died");
}

GbExec::GbExec()
{
    const sp<IGbExecService>& service = IGbExecDeathNotifier::getGbExecService();
    if (service != 0) {
        GLOGE(LOG_TAG, "getGbExecService successed!");
        // sp<IGbExec> exec(service->create());
        // if (exec == NULL)
        // {
        //  GLOGE(LOG_TAG, "service->create() failed!");
        //  return;
        // }
        // mExec = exec;
        return;
    }
    GLOGE(LOG_TAG, "getGbExecService failed!");
}

GbExec::~GbExec()
{
    GLOGE(LOG_TAG, "~GbExec In");
    // disconnect();
    GLOGE(LOG_TAG, "~GbExec Out");
}

// void GbExec::disconnect()
// {
//     GLOGE(LOG_TAG, "GbExec::disconnect() In");
//     sp<IGbExec> p;
//     {
//         // Mutex::Autolock _l(mLock);
//         p = mExec;
//         mExec.clear();
//     }

//     if (p != 0) {
//         p->disconnect();
//         p = 0;
//     }
//     GLOGE(LOG_TAG, "GbExec::disconnect() Out");
// }

void GbExec::Kill(const char* pid)
{
    const sp<IGbExecService>& service = IGbExecDeathNotifier::getGbExecService();
    if (service != 0)
    {
        GLOGE(LOG_TAG, "kill getGbExecService successed!");
        service->kill(pid);
        return;
    }
    GLOGE(LOG_TAG, "kill getGbExecService failed!");
}

}; // namespace android