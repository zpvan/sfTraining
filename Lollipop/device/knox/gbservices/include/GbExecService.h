#ifndef GB_EXEC_SERVICE_H
#define GB_EXEC_SERVICE_H

#include "IGbExecService.h"

namespace android {

class GbExecService : public BnGbExecService {

public:
    static void instantiate();

    virtual void kill(const char* i_pid);

private:
                            GbExecService();
    virtual                 ~GbExecService();

};

}; // namespace android

#endif // GB_EXEC_SERVICE_H