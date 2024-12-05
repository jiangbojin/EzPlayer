#ifndef COMMONLOOPER_H
#define COMMONLOOPER_H

#include <thread>

#include<ff_ffplay_def.h>
#include<log/easylogging++.h>
#include <functional>
#include<iostream>
#include <memory>

enum class RET_CODE;

class CommonLooper
{
private:
    static void* trampoline(void* p);

protected:
    std::unique_ptr<std::thread> worker_;
    bool request_exit_ = false;
    bool running_ = false;

public:


    CommonLooper()=default;
    virtual ~CommonLooper(){
        if(running_)
        {
            LOG(DEBUG)<<("CommonLooper deleted while still running. Some messages will not be processed");
            Stop();
        }
    }

    ///开启事件循环机制
    ///
    /// \return
    ///
    virtual RET_CODE Start();
    ///
    ///可多次调用
    ///
    virtual void Stop();
    ///
    ///子类实现事件循环
    ///
    virtual void Loop() = 0;
    void setRunning(bool newRunning);
    bool getRunning(){
        return running_;
    }

};

#endif // COMMONLOOPER_H
