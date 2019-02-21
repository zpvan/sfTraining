#define LOG_TAG "GbIgmpService"

#include <GbIgmpService.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include "igmp.h"
#include "GbLog.h"
#include <cutils/atomic.h>

namespace android
{

void GbIgmpService::instantiate() 
{
    // GLOGE(LOG_TAG, "instantiate");
    defaultServiceManager()->addService(
            String16("gb.igmp.server"), new GbIgmpService());
}

GbIgmpService::GbIgmpService()
{
    // GLOGE(LOG_TAG, "constructor");
    mNextConnId = 1;
}

GbIgmpService::~GbIgmpService()
{
    // GLOGE(LOG_TAG, "destructor");
}

sp<IGbIgmp> GbIgmpService::create()
{
    pid_t pid = IPCThreadState::self()->getCallingPid();
    int32_t connId = android_atomic_inc(&mNextConnId);

    sp<Client> c = new Client(this);

    // GLOGE(LOG_TAG, "Create new client(%d) from pid %d, uid %d", connId, pid,
    //      IPCThreadState::self()->getCallingUid());

    wp<Client> w = c;
    {
        Mutex::Autolock lock(mLock);
        mClients.add(w);
    }
    return c;
}

void GbIgmpService::removeClient(wp<Client> client)
{
    // GLOGE(LOG_TAG, "removeClient");
    Mutex::Autolock lock(mLock);
    mClients.remove(client);
}

GbIgmpService::Client::Client(const sp<GbIgmpService>& service)
{
    // GLOGE(LOG_TAG, "GbIgmpService::Client:Client() In");
    mService = service;
    mCigmp = new CIGMP();
    // GLOGE(LOG_TAG, "GbIgmpService::Client:Client() Out"); 
}

GbIgmpService::Client::~Client()
{
    // GLOGE(LOG_TAG, "GbIgmpService::Client destructor In");
    wp<Client> client(this);
    // disconnect();
    if (mService != NULL)
    {
        mService->removeClient(client);
    }

    if (mCigmp != 0)
    {
        delete mCigmp;
        mCigmp = 0;
    }
    // IPCThreadState::self()->flushCommands();
    // GLOGE(LOG_TAG, "GbIgmpService::Client destructor Out");
}

// void GbIgmpService::Client::disconnect()
// {
//     GLOGE(LOG_TAG, "GbIgmpService::Client disconnect In");
//     if (mCigmp != 0)
//     {
//         delete mCigmp;
//         mCigmp = 0;
//     }
//     // IPCThreadState::self()->flushCommands();
//     GLOGE(LOG_TAG, "GbIgmpService::Client disconnect Out");
// }

void GbIgmpService::Client::Open(const char* i_pLocalAddr, const char* i_pMulticastAddr)
{
    // GLOGE(LOG_TAG, "GbIgmpService::Client:Open() In");
    if (mCigmp != NULL)
    {
        const char *la = i_pLocalAddr;
        const char *ma = i_pMulticastAddr;
        GLOGE(LOG_TAG, "mCigmp open [la, ma]=[%s, %s]", la, ma);
        mCigmp->Open(la, ma);
    }
    // GLOGE(LOG_TAG, "GbIgmpService::Client:Open() Out");
}

void GbIgmpService::Client::MemberShip(void)
{
    // GLOGE(LOG_TAG, "GbIgmpService::Client:MemberShip() In");
    if (mCigmp != NULL)
    {
        // GLOGD("Cigmp_native_memberShip");
        // GLOGE(LOG_TAG, "mCigmp memberShip");
        mCigmp->MemberShip();
    }
    // GLOGE(LOG_TAG, "GbIgmpService::Client:MemberShip() Out");

}

void GbIgmpService::Client::Close(void)
{
    // GLOGE(LOG_TAG, "GbIgmpService::Client:Close() In");
    if (mCigmp != NULL)
    {
        // GLOGE(LOG_TAG, "mCigmp close");
        mCigmp->Close();
    }
    // GLOGE(LOG_TAG, "GbIgmpService::Client:Close() Out");
}

}; // namespace android