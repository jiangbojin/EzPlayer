#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H
#include <mutex>
#include <condition_variable>
#include <list>

#define MSG_FLUSH                   1
#define MSG_RTSP_ERROR              100
#define MSG_RTSP_QUEUE_DURATION     101
typedef struct AVMessage{
    int what=0;           // 消息类型
    int arg1=0;           // 参数1
    int arg2=0;           // 参数2

    void *obj =nullptr;         //参数扩容 传入结构体
    void(*free_l)(void * obj);
}AVMessage;

///
/// 参考ijkplay的消息队列，主要解决解耦问题。
/// 扩展功能：
/// 1.加入循环使用机制，减少内存分配次数，提高性能
/// 2.支持超时等待，支持消息队列中消息的删除
///
class MessageQueue
{
    std::mutex mutex_;
    std::condition_variable cond_;
    std::list<AVMessage *> queue_;

    int abort_request_ = 0;

    int msg_queue_push_back(AVMessage *msg);
    inline void msg_init_msg(AVMessage * msg);
    ///
    /// 插入一条消息
    /// \param msg
    /// \return 正常返回0
    ///
    int msg_queue_put(AVMessage* msg);
    ///
    /// 清空消息队列
    ///
    void msg_queue_flush();
public:
    MessageQueue()= default;
    ~MessageQueue(){    msg_queue_flush();  }

    void msg_queue_abort(){
        std::unique_lock<std::mutex> lock(mutex_);
        abort_request_ = 1;
    }
    void msg_queue_start(){
        std::unique_lock<std::mutex> lock(mutex_);
        abort_request_ = 0;
    }
    ///
    ///
    /// \param msg  接收返回msg
    /// 如果msg->obj有值，接收者需自行处理调用 msg->free_l(msg->obj);
    /// \param timeout  超时时间ms
    /// \return -2表示参数异常;-1 表示 abort终止; 0表示超时没有消息; 1表示成功get到消息
    ///
    int msg_queue_get(AVMessage * msg,int timeout);
    ///
    /// 删除队列中所有的指定消息what
    /// \param what
    /// \return
    ///
    int msg_queue_remove(int what);
    ///
    /// 插入一条消息到队列中
    /// \param what
    /// \param arg1
    /// \param arg2
    /// \param obj
    /// \param obj_len
    ///
    void notify_msg(int what, int arg1 = 0, int arg2 = 0, void* obj = nullptr, int obj_len = 0);


    void msg_queue_destroy(){   msg_queue_flush();  }
};

#endif // MESSAGEQUEUE_H
