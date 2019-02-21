
#ifndef GB_IGMP_H
#define GB_IGMP_H

#include "IGbIgmpService.h"
#include "IGbIgmp.h"
#include <utils/threads.h>
#include <utils/SortedVector.h>

namespace android {

class IGbIgmpDeathNotifier: virtual public RefBase
{
public:
    IGbIgmpDeathNotifier() { 
        addObitRecipient(this);
    }
    virtual ~IGbIgmpDeathNotifier() { 
        removeObitRecipient(this);
    }

    virtual void died() = 0;
    static const sp<IGbIgmpService>& getGbIgmpService();

private:
    IGbIgmpDeathNotifier &operator=(const IGbIgmpDeathNotifier &);
    IGbIgmpDeathNotifier(const IGbIgmpDeathNotifier &);

    static void addObitRecipient(const wp<IGbIgmpDeathNotifier>& recipient);
    static void removeObitRecipient(const wp<IGbIgmpDeathNotifier>& recipient);

    class DeathNotifier: public IBinder::DeathRecipient
    {
    public:
                DeathNotifier() {}
        virtual ~DeathNotifier();

        virtual void binderDied(const wp<IBinder>& who);
    };

    friend class DeathNotifier;

    static  Mutex                                    sServiceLock;
    static  sp<IGbIgmpService>                       sGbIgmpService;
    static  sp<DeathNotifier>                        sDeathNotifier;
    static  SortedVector< wp<IGbIgmpDeathNotifier> > sObitRecipients;
};

class GbIgmp: public virtual IGbIgmpDeathNotifier
{
public:
	// static const sp<IGbIgmpService>& getGbIgmpService();

	GbIgmp();
	~GbIgmp();

	void died();
    // void disconnect();

	void Open(const char* i_pLocalAddr, const char* i_pMulticastAddr);
	void MemberShip(void);
	void Close(void);

private:
	sp<IGbIgmp> mIgmp;
};

}; // namespace android

#endif // GB_IGMP_H