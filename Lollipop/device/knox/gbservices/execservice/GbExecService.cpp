#define LOG_TAG "GbExecService"

#include "GbLog.h"
#include "GbExecService.h"
#include <unistd.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

namespace android
{

void GbExecService::instantiate() 
{
    // GLOGE(LOG_TAG, "instantiate");
    defaultServiceManager()->addService(
            String16("gb.exec.server"), new GbExecService());
}

static int strlen(const char *str)
{
    int len = 0;
    assert(str != NULL);
    while(*str++ != '\0')
    {
        len++;
    }
    return len;
}

void *run_kill(void *data) {
    char *pid = (char *)data; 
    GLOGE(LOG_TAG, "run_kill %s %d In", pid, strlen(pid));
    char *argv[ ] = {const_cast<char *>("kill"), pid, 0};
    execvp("kill", argv);
    GLOGE(LOG_TAG, "run_kill %s Out", pid);
    return 0;
} 

void GbExecService::kill(const char *i_pid)
{
    GLOGE(LOG_TAG, "kill In");
    pid_t fpid;
    int size = strlen(i_pid);
    char *pid = new char[size + 1];
    strcpy(pid, i_pid);
    // char *pid = const_cast<char *>(i_pid);
    fpid = fork();
    if (fpid < 0) {
        GLOGE(LOG_TAG, "kill fork failed!");
    } else if (fpid == 0) {
        GLOGE(LOG_TAG, "kill fork(%d) success!", getpid());
        pthread_t thread;
        pthread_create(&thread, NULL, &run_kill, (void *)pid);
        pthread_join(thread, NULL);
    } else {  
        GLOGE(LOG_TAG, "kill myself(%d)", getpid());
    }
    delete pid;
    pid = NULL;
    GLOGE(LOG_TAG, "kill Out");
}

GbExecService::GbExecService()
{
    // GLOGE(LOG_TAG, "constructor");
}

GbExecService::~GbExecService()
{
    // GLOGE(LOG_TAG, "destructor");
}

}; // namespace android