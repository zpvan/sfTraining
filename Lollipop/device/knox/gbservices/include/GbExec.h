#ifndef GB_EXEC_H
#define GB_EXEC_H

#include "IGbExecService.h"
// #include "IGbExec.h"
#include <utils/threads.h>
#include <utils/SortedVector.h>

namespace android {

class IGbExecDeathNotifier: virtual public RefBase
{
public:
    IGbExecDeathNotifier() { 
        addObitRecipient(this);
    }
    virtual ~IGbExecDeathNotifier() { 
        removeObitRecipient(this);
    }

    virtual void died() = 0;
    static const sp<IGbExecService>& getGbExecService();

private:
    IGbExecDeathNotifier &operator=(const IGbExecDeathNotifier &);
    IGbExecDeathNotifier(const IGbExecDeathNotifier &);

    static void addObitRecipient(const wp<IGbExecDeathNotifier>& recipient);
    static void removeObitRecipient(const wp<IGbExecDeathNotifier>& recipient);

    class DeathNotifier: public IBinder::DeathRecipient
    {
    public:
                DeathNotifier() {}
        virtual ~DeathNotifier();

        virtual void binderDied(const wp<IBinder>& who);
    };

    friend class DeathNotifier;

    static  Mutex                                    sServiceLock;
    static  sp<IGbExecService>                       sGbExecService;
    static  sp<DeathNotifier>                        sDeathNotifier;
    static  SortedVector< wp<IGbExecDeathNotifier> > sObitRecipients;
};

class GbExec: public virtual IGbExecDeathNotifier
{
public:
	// static const sp<IGbExecService>& getGbExecService();

	GbExec();
	~GbExec();

	void died();
    // void disconnect();

	void Kill(const char* pid);

private:
	// sp<IGbExec> mExec;
};

}; // namespace android

#endif // GB_EXED_H