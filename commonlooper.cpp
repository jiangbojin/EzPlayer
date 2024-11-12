#include "commonlooper.h"

#include"log/easylogging++.h"
RET_CODE CommonLooper::Start()
{
    LOG(DEBUG)<<"RET_CODE CommonLooper::Start()";
    worker_ = std::move(std::make_unique<std::thread>(CommonLooper::trampoline,this));
    if(worker_ == NULL)
    {
        LOG(ERROR)<<"new std::thread failed";
        return RET_FAIL;
    }
    return RET_OK;
}

void CommonLooper::Stop()
{
    request_exit_ = true;
    if(worker_ && worker_.get() && worker_.get()->joinable())
    {
        worker_.get()->join();
        worker_.reset();
    }
    running_ = false;
}

void *CommonLooper::trampoline(void *p)
{
    LOG(DEBUG)<<"at CommonLooper trampoline";

    ((CommonLooper*)p)->setRunning(true);
    //多态调用 SetCallback
    ((CommonLooper*)p)->Loop();
    ((CommonLooper*)p)->setRunning(false);
    return NULL;
}

void CommonLooper::setRunning(bool newRunning)
{
    running_ = newRunning;
}
