/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "gbserver"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

// from LOCAL_C_INCLUDES
#include "GbLog.h"
#include "GbIgmpService.h"
#include "GbExecService.h"

using namespace android;

int main()
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();
    // GLOGE(LOG_TAG, "ServiceManager: %p", sm.get());
    GbIgmpService::instantiate();
    GbExecService::instantiate();
    // GLOGE(LOG_TAG, "instantiate: done");
    ProcessState::self()->startThreadPool();
    // GLOGE(LOG_TAG, "startThreadPool: done");
    IPCThreadState::self()->joinThreadPool();
    // GLOGE(LOG_TAG, "joinThreadPool: done");
}
