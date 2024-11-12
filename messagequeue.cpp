#include<messagequeue.h>
#include<algorithm>
#include "easylogging++.h"
extern "C"
{
#include "libavcodec/avcodec.h"
}
static void msg_obj_free_l(void *obj)
{
    //释放操作xxx


    av_free(obj);
}


int MessageQueue::msg_queue_push_back(AVMessage *msg)
{
    std::unique_lock<std::mutex>lock(mutex_);
    if(abort_request_)
        return -1;

    AVMessage * temp = (AVMessage*) av_malloc(sizeof (AVMessage));
    if(!temp)
        return -1;
    *temp = *msg;
    queue_.emplace_back(temp);
    return 0;
}

inline void MessageQueue::msg_init_msg(AVMessage* msg)
{
    memset(msg, 0, sizeof(AVMessage));
}

int MessageQueue::msg_queue_put(AVMessage *msg)
{

    LOG(DEBUG) << "msg_queue_put test";
    int ret = msg_queue_push_back(msg);
    if(ret == 0){
        cond_.notify_one();
    }
    return ret;
}
void MessageQueue::notify_msg(int what, int arg1, int arg2, void *obj, int obj_len)
{
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg.arg2 = arg2;

    if(obj && obj_len > 0){
        msg.obj = av_malloc(obj_len);
        msg.free_l = msg_obj_free_l;
        memcpy_s(msg.obj,obj_len,obj,obj_len);
    }
    msg_queue_put(&msg);
}

///
///
/// \param msg  接收返回msg
/// 如果msg->obj有值，接收者需自行处理调用 msg->free_l(msg->obj);
/// \param timeout  超时时间ms
/// \return -2表示参数异常;-1 表示 abort终止; 0表示超时没有消息; 1表示成功get到消息
///
int MessageQueue::msg_queue_get(AVMessage *msg, int timeout)
{
    if(!msg || timeout < 0)
        return -2;
    std::unique_lock<std::mutex>lock(mutex_);

    AVMessage* result;

    if(abort_request_)
        return -1;
    if(queue_.empty())
        return 0;
    if(timeout > 0){
        if (std::cv_status::no_timeout !=
                (std::cv_status)cond_.wait_for(lock,std::chrono::milliseconds(timeout)
                             ,[this](){return !abort_request_ && !queue_.empty();})) {
            // 等待超时
            return 0;
        }

    }else{   //永久等待
        cond_.wait(lock,[this](){
            return !abort_request_ && !queue_.empty();
        });
    }

    //数据处理
    result = queue_.front();
    *msg = *result;
    queue_.pop_front();
    av_free(result);

    return 1;
}

int MessageQueue::msg_queue_remove(int what)
{
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.remove_if([what](AVMessage* msg){
        if(msg->what == what){
            if(msg->obj && msg->free_l){
                msg->free_l(msg->obj);
            }
            av_free((void*)msg);
            return true;
        }
        return false;

    });
    return 0;
}


void MessageQueue::msg_queue_flush()
{
    std::unique_lock <std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        AVMessage * msg = queue_.front();
        if(msg->obj && msg->free_l)
            msg->free_l(msg->obj);
        queue_.pop_front();
        av_free((void*)msg);
    }
}
