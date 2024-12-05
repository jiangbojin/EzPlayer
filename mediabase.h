/*
 *****************************************************************************
 * @author     c9程序员<2655609674@qq.com><qq:2655609674>
 * @date       2024/12/02
 * @file       mediabase.h
 * @history
 *
 * @file core rtmp解包所需要的类型，以及基本配置。
 ****************************************************************************
*/

#ifndef MEDIABASE_H
#define MEDIABASE_H

#ifndef _MSC_VER
#include <strings.h>
#endif
#include <log/easylogging++.h>
#include <map>
#include <vector>
#include <sstream>
#include <cstdint>
#include <cstddef>
extern "C"{
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "librtmp/rtmp.h"
}

class mediabase
{
public:
    mediabase();
};




#define NALU_TYPE_SLICE 1
#define NALU_TYPE_DPA 2
#define NALU_TYPE_DPB 3
#define NALU_TYPE_DPC 4
#define NALU_TYPE_IDR 5 //I帧
#define NALU_TYPE_SEI 6
#define NALU_TYPE_SPS 7
#define NALU_TYPE_PPS 8
#define NALU_TYPE_AUD 9 //访问分隔符
#define NALU_TYPE_EOSEQ 10
#define NALU_TYPE_EOSTREAM 11
#define NALU_TYPE_FILL 12

#define NALU_FRAME_I 15
#define NALU_FRAME_P 16
#define NALU_FRAME_B 17

/////// profile
#define	QCIF	0	// 176  x 144 	AR:	1,222222222
#define	CIF	1	// 352  x 288	AR:	1,222222222
#define	VGA	2	// 640  x 480	AR:	1,333333333
#define	PAL	3	// 768  x 576	AR:	1,333333333
#define	HVGA	4	// 480  x 320	AR:	1,5
#define	QVGA	5	// 320  x 240	AR:	1,333333333
#define	HD720P	6	// 1280 x 720	AR:	1,777777778
#define	WQVGA	7	// 400  x 240	AR:	1,666666667
#define	W448P	8	// 768  x 448	AR:	1,714285714
#define	SD448P	9	// 576  x 448	AR:	1,285714286
#define	W288P	10	// 512  x 288	AR:	1,777777778
#define	W576	11	// 1024 x 576	AR:	1,777777778
#define	FOURCIF	12	// 704  x 576	AR:	1,222222222
#define	FOURSIF	13	// 704  x 480	AR:	1,466666667
#define	XGA	14	// 1024 x 768	AR:	1,333333333
#define	WVGA	15	// 800  x 480	AR:	1,666666667
#define	DCIF	16	// 528  x 384	AR:	1,375
#define	SIF	17	// 352  x 240	AR:	1,466666667
#define	QSIF	18	// 176  x 120	AR:	1,466666667
#define	SD480P	19	// 480  x 360	AR:	1,333333333
#define	SQCIF	20	// 128  x 96	AR:	1,333333333
#define	SCIF	21	// 256  x 192	AR:	1,333333333
#define	HD1080P	22	// 1920 x 1080  AR:     1,777777778
#define UW720P  23	// 1680 x 720   AR:	2,333333333

inline uint32_t GetWidth(uint32_t size)
{
    //Depending on size
    switch(size)
    {
    case QCIF:	return 176;
    case CIF:	return 352;
    case VGA:	return 640;
    case PAL:	return 768;
    case HVGA:	return 480;
    case QVGA:	return 320;
    case HD720P:	return 1280;
    case WQVGA:	return 400;
    case W448P:	return 768;
    case SD448P:	return 576;
    case W288P:	return 512;
    case W576:	return 1024;
    case FOURCIF:	return 704;
    case FOURSIF:	return 704;
    case XGA:	return 1024;
    case WVGA:	return 800;
    case DCIF:	return 528;
    case SIF:	return 352;
    case QSIF:	return 176;
    case SD480P:	return 480;
    case SQCIF:	return 128;
    case SCIF:	return 256;
    case HD1080P:	return 1920;
    case UW720P:	return 1680;
    }
    //Nothing
    return 0;
}
inline uint32_t GetHeight(uint32_t size)
{
    //Depending on size
    switch(size)
    {
    case QCIF:	return 144;
    case CIF:	return 288;
    case VGA:	return 480;
    case PAL:	return 576;
    case HVGA:	return 320;
    case QVGA:	return 240;
    case HD720P:	return 720;
    case WQVGA:	return 240;
    case W448P:	return 448;
    case SD448P:	return 448;
    case W288P:	return 288;
    case W576:	return 576;
    case FOURCIF:	return 576;
    case FOURSIF:	return 480;
    case XGA:	return 768;
    case WVGA:	return 480;
    case DCIF:	return 384;
    case SIF:	return 240;
    case QSIF:	return 120;
    case SD480P:	return 360;
    case SQCIF:	return 96;
    case SCIF:	return 192;
    case HD1080P:	return 1080;
    case UW720P:	return 720;
    }

    return 0;
}



class FileTag {
public:
    unsigned char type;        // 标签类型(8/9/18分别代表音频/视频/脚本数据)
    unsigned int nTimeStamp;   // 时间戳(毫秒)
    unsigned int nStreamID;    // 流ID，通常为0
    unsigned char * data;      // 标签数据
    unsigned int len;          // 数据长度
    bool haveAudio;           // 是否包含音频
    bool haveVideo;           // 是否包含视频

    unsigned int totalSize;    // 总大小
    unsigned int totalTime;    // 总时长
    float percent;             // 百分比(可能用于播放进度)


};
///
/// H.264视频编码配置记录
///
class AVCDecoderConfigurationRecord {
public:
    uint8_t configurationVersion;     // 配置版本号，固定为1
    uint8_t avcProfileIndication;     // H.264 Profile
    uint8_t profile_compatibility;    // Profile兼容性
    uint8_t avcLevelIndication;       // H.264 Level
    uint8_t lengthSizeMinusOne;       // NALU长度字段的字节数减1

    // SPS(序列参数集)相关
    uint8_t numOfSequenceParameterSets;    // SPS个数
    uint16_t sequenceParameterSetLength;    // SPS长度
    uint8_t * sequenceParameterSetNALUnits; // SPS数据

    // PPS(图像参数集)相关
    uint8_t numOfPictureParameterSets;      // PPS个数
    uint16_t pictureParameterSetLength;      // PPS长度
    uint8_t * pictureParameterSetNALUits;    // PPS数据
};

///
/// AAC音频配置
///
class FLV_AudioSpecificConfig {
public:
    uint8_t audioObjectType : 5;           // 音频编码类型(AAC-LC=2)
    uint8_t samplingFrequencyIndex : 4;    // 采样率索引
    uint8_t channelConfiguration : 4;       // 声道配置

    // GA(General Audio)特定配置
    typedef struct {
        uint8_t frameLengthFlag : 1;     // 帧长度标志
        uint8_t dependsOnCoreCoder : 1;  // 是否依赖核心解码器
        uint8_t extensionFlag : 1;       // 扩展标志
    } GASpecificConfig;

    GASpecificConfig gaSpecificConfig;
};

enum MEIDA_BASE_PIX_FMT {
    MEIDA_BASE_PIX_FMT_NONE = -1,
    MEIDA_BASE_PIX_FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    MEIDA_BASE_PIX_FMT_YUYV422,   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    MEIDA_BASE_PIX_FMT_RGB24,     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
    MEIDA_BASE_PIX_FMT_BGR24,     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
};
enum CB_EVENT{
    // 音频事件
    EVT_AUD_PKT_CACHE_ENOUGH,
    EVT_AUD_UNDER_RUN,      // 音频缺乏数据输出


};
enum class RET_CODE
{
    RET_ERR_UNKNOWN = -2,                   // 未知错误
    RET_FAIL = -1,							// 失败
    RET_OK	= 0,							// 正常
    RET_ERR_OPEN_FILE,						// 打开文件失败
    RET_ERR_NOT_SUPPORT,					// 不支持
    RET_ERR_OUTOFMEMORY,					// 没有内存
    RET_ERR_STACKOVERFLOW,					// 溢出
    RET_ERR_NULLREFERENCE,					// 空参考
    RET_ERR_ARGUMENTOUTOFRANGE,				//
    RET_ERR_PARAMISMATCH,					//
    RET_ERR_MISMATCH_CODE,                  // 没有匹配的编解码器
    RET_ERR_EAGAIN,
    RET_ERR_EOF
};
//Scale算法
enum class SwsAlogrithm
{
    SWS_SA_FAST_BILINEAR    = 0x1,
    SWS_SA_BILINEAR            = 0x2,
    SWS_SA_BICUBIC            = 0x4,
    SWS_SA_X                = 0x8,
    SWS_SA_POINT            = 0x10,
    SWS_SA_AREA                = 0x20,
    SWS_SA_BICUBLIN            = 0x40,
    SWS_SA_GAUSS            = 0x80,
    SWS_SA_SINC                = 0x100,
    SWS_SA_LANCZOS            = 0x200,
    SWS_SA_SPLINE            = 0x400,
};


class VideoFrame
{
public:
    uint8_t *data[8] = {NULL};         // 类似FFmpeg的buf, 如果是
    int32_t linesize[8] = {0};
    int32_t width;
    int32_t height;
    int format = MEIDA_BASE_PIX_FMT_YUV420P;
};

class TimesUtil
{
public:
    static inline int64_t GetTimeMillisecond()
    {
#ifdef _WIN32
        return (int64_t)GetTickCount();
#else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return ((int64_t)tv.tv_sec * 1000 + (unsigned long long)tv.tv_usec / 1000);
#endif
    }
};










///
/// 参数传递类
///
class Properties: public std::map<std::string,std::string>
{
public:
#ifdef _MSC_VER
    // Windows平台下的字符串比较函数实现
    static inline int strcasecmp(const char *s1, const char *s2)
    {
        //   while  (toupper((unsigned char)*s1) == toupper((unsigned char)*s2++))
        //       if (*s1++ == '\0') return 0;
        //   return(toupper((unsigned char)*s1) - toupper((unsigned char)*--s2));
        while  ((unsigned char)*s1 == (unsigned char)*s2++)
            if (*s1++ == '\0') return 0;
        return((unsigned char)*s1 - (unsigned char)*--s2);
    }

#endif

    bool HasProperty(const std::string &key) const
    {
        return find(key)!=end();
    }

    void SetProperty(const char* key,int intval)
    {
        SetProperty(std::string(key),std::to_string(intval));
    }

    void SetProperty(const char* key,uint32_t val)
    {
        SetProperty(std::string(key),std::to_string(val));
    }

    void SetProperty(const char* key,uint64_t val)
    {
        SetProperty(std::string(key),std::to_string(val));
    }

    void SetProperty(const char* key,const char* val)
    {
        SetProperty(std::string(key),std::string(val));
    }

    void SetProperty(const std::string &key,const std::string &val)
    {
        insert(std::pair<std::string,std::string>(key,val));
    }

    void SetProperty(const char* key, float val)
    {
        SetProperty(std::string(key),std::to_string(val));
    }

    void GetChildren(const std::string& path,Properties &children) const
    {
        //Create sarch string
        std::string parent(path);
        //Add the final .
        parent += ".";
        //For each property
        for (const_iterator it = begin(); it!=end(); ++it)
        {
            const std::string &key = it->first;
            //Check if it is from parent
            if (key.compare(0,parent.length(),parent)==0)
                //INsert it
                children.SetProperty(key.substr(parent.length(),key.length()-parent.length()),it->second);
        }
    }

    void GetChildren(const char* path,Properties &children) const
    {
        GetChildren(std::string(path),children);
    }

    Properties GetChildren(const std::string& path) const
    {
        Properties properties;
        //Get them
        GetChildren(path,properties);
        //Return
        return properties;
    }

    Properties GetChildren(const char* path) const
    {
        Properties properties;
        //Get them
        GetChildren(path,properties);
        //Return
        return properties;
    }

    void GetChildrenArray(const char* path,std::vector<Properties> &array) const
    {
        //Create sarch string
        std::string parent(path);
        //Add the final .
        parent += ".";

        //Get array length
        int length = GetProperty(parent+"length",0);

        //For each element
        for (int i=0; i<length; ++i)
        {
            char index[64];
            //Print string
            snprintf(index,sizeof(index),"%d",i);
            //And get children
            array.push_back(GetChildren(parent+index));
        }
    }

    const char* GetProperty(const char* key) const
    {
        return GetProperty(key,"");
    }

    std::string GetProperty(const char* key,const std::string defaultValue) const
    {
        //Find item
        const_iterator it = find(std::string(key));
        //If not found
        if (it==end())
            //return default
            return defaultValue;
        //Return value
        return it->second;
    }

    std::string GetProperty(const std::string &key,const std::string defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it==end())
            //return default
            return defaultValue;
        //Return value
        return it->second;
    }

    const char* GetProperty(const char* key,const char *defaultValue) const
    {
        //Find item
        const_iterator it = find(std::string(key));
        //If not found
        if (it==end())
            //return default
            return defaultValue;
        //Return value
        return it->second.c_str();
    }

    const char* GetProperty(const std::string &key,char *defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it==end())
            //return default
            return defaultValue;
        //Return value
        return it->second.c_str();
    }

    int GetProperty(const char* key,int defaultValue) const
    {
        return GetProperty(std::string(key),defaultValue);
    }

    int GetProperty(const std::string &key,int defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it==end())
            //return default
            return defaultValue;
        //Return value
        return atoi(it->second.c_str());
    }

    uint64_t GetProperty(const char* key,uint64_t defaultValue) const
    {
        return GetProperty(std::string(key),defaultValue);
    }

    uint64_t GetProperty(const std::string &key,uint64_t defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it==end())
            //return default
            return defaultValue;
        //Return value
        return atoll(it->second.c_str());
    }

    bool GetProperty(const char* key,bool defaultValue) const
    {
        return GetProperty(std::string(key),defaultValue);
    }

    bool GetProperty(const std::string &key,bool defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it==end())
            //return default
            return defaultValue;
        //Get value
        char * val = (char *)it->second.c_str();
        //Check it
        if (strcasecmp(val,(char *)"yes")==0)
            return true;
        else if (strcasecmp(val,(char *)"true")==0)
            return true;
        //Return value
        return bool((atoi(val)));
    }

    float GetProperty(const char* key,float defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it==end())
            //return default
            return defaultValue;
        //Return value
        return atof(it->second.c_str());
    }
};


inline void* malloc32(size_t size)
{
    void* ptr = malloc(size);
    if(!ptr)
        return NULL;
    return ptr;
}
/**
 * @brief 字节缓冲区类，用于管理可动态增长的字节数组
主要功能：
Alloc(const uint32_t size)
Set(const uint8_t* data,const uint32_t size)
Append(const uint8_t* data,const uint32_t size)
 */
class ByteBuffer
{
public:
    /**
     * @brief 默认构造函数，创建空缓冲区
     */
    ByteBuffer()
    {
        size = 0;      // 缓冲区容量
        buffer = NULL; // 缓冲区指针
        length = 0;    // 实际数据长度
    }

    /**
     * @brief 指定大小的构造函数
     * @param size 要分配的缓冲区大小
     */
    ByteBuffer(const uint32_t size)
    {
        length = 0;
        this->size = size;
        buffer = (uint8_t*) malloc32(size);
    }

    /**
     * @brief 使用已有数据创建缓冲区
     * @param data 源数据指针
     * @param size 数据大小
     */
    ByteBuffer(const uint8_t* data,const uint32_t size)
    {
        this->size = size;
        buffer = (uint8_t*) malloc32(size);
        memcpy(buffer,data,size);
        length=size;
    }

    /**
     * @brief 从另一个ByteBuffer指针复制创建
     * @param bytes 源ByteBuffer指针
     */
    ByteBuffer(const ByteBuffer* bytes)
    {
        size = bytes->GetLength();
        buffer = (uint8_t*) malloc32(size);
        memcpy(buffer,bytes->GetData(),size);
        length=size;
    }

    /**
     * @brief 复制构造函数
     * @param bytes 源ByteBuffer对象
     */
    ByteBuffer(const ByteBuffer& bytes)
    {
        size = bytes.GetLength();
        buffer = (uint8_t*) malloc32(size);
        memcpy(buffer,bytes.GetData(),size);
        length=size;
    }

    /**
     * @brief 克隆当前缓冲区
     * @return 返回新的ByteBuffer指针
     */
    ByteBuffer* Clone() const {
        return new ByteBuffer(buffer,length);
    }

    /**
     * @brief 析构函数，释放分配的内存
     */
    virtual ~ByteBuffer()
    {
        if(buffer) free(buffer);
    }

    /**
     * @brief 重新分配缓冲区大小
     * @param size 新的缓冲区大小
     */
    void Alloc(const uint32_t size)
    {
        this->size = size;
        buffer = (uint8_t*) realloc(buffer,size);
    }

    /**
     * @brief 设置缓冲区数据
     * @param data 源数据指针
     * @param size 数据大小
     */
    void Set(const uint8_t* data,const uint32_t size)
    {
        // 如果需要更大空间，扩容至1.5倍
        if (size>this->size)
            Alloc(size*3/2);
        memcpy(buffer,data,size);
        length=size;
    }

    /**
     * @brief 追加数据到缓冲区末尾
     * @param data 要追加的数据
     * @param size 追加数据的大小
     * @return 返回追加前的位置
     */
    uint32_t Append(const uint8_t* data,const uint32_t size)
    {
        uint32_t pos = length;
        // 如果需要更大空间，扩容至1.5倍
        if (size+length>this->size)
            Alloc((size+length)*3/2);
        memcpy(buffer+length,data,size);
        length+=size;
        return pos;
    }

    /**
     * @brief 获取缓冲区数据指针
     * @return 返回只读数据指针
     */
    const uint8_t* GetData() const
    {
        return buffer;
    }

    /**
     * @brief 获取缓冲区容量
     * @return 返回缓冲区容量
     */
    uint32_t GetSize() const
    {
        return size;
    }

    /**
     * @brief 获取实际数据长度
     * @return 返回数据长度
     */
    uint32_t GetLength() const
    {
        return length;
    }

protected:
    uint8_t	*buffer;  // 数据缓冲区指针
    uint32_t length;  // 实际数据长度
    uint32_t size;    // 缓冲区容量
};



// 基础消息对象类，作为所有消息类的基类 通过post调用
class MsgBaseObj {
public:
    MsgBaseObj(){}
    virtual ~MsgBaseObj(){} // 虚析构函数确保正确释放子类资源
};


// 循环消息结构体定义，用于消息循环系统
class LooperMessage {
 public:
    int what;           // 消息类型标识
    MsgBaseObj *obj;    // 消息携带的对象指针
    bool quit;          // 退出标志
};

// RTMP消息体类型枚举
enum RTMP_BODY_TYPE {
    RTMP_BODY_METADATA,  // 元数据
    RTMP_BODY_AUD_RAW,   // 音频原始数据
    RTMP_BODY_AUD_SPEC,  // 音频规格配置
    RTMP_BODY_VID_RAW,   // 视频原始数据
    RTMP_BODY_VID_CONFIG // H264配置数据
};

// YUV格式数据基础结构
class YUVStruct : public MsgBaseObj {
public:
    int size = 0;      // 数据大小
    int width = 0;     // 图像宽度
    int height = 0;    // 图像高度
    char *data = NULL; // 图像数据
    YUVStruct(int size, int width, int height);
    YUVStruct(char*data, int size, int width, int height);
    virtual ~YUVStruct();
};


// YUV420p格式数据结构
class YUV420p : public YUVStruct {
public:
    char* Y;  // Y平面数据
    char* U;  // U平面数据
    char* V;  // V平面数据

    YUV420p(int32_t size, int32_t width, int32_t height);
    YUV420p(char* data, int32_t size, int32_t width, int32_t height);
    virtual ~YUV420p();
};

// FLV元数据消息类
class FLVMetadataMsg: public MsgBaseObj {
public:
    FLVMetadataMsg(){}
    virtual ~FLVMetadataMsg(){}

    bool has_audio = false;     // 是否包含音频
    bool has_video = false;     // 是否包含视频
    int audiocodeid = -1;       // 音频编码ID
    int audiodatarate = 0;      // 音频码率
    int audiodelay = 0;         // 音频延迟
    int audiosamplerate = 0;    // 音频采样率
    int audiosamplesize = 0;    // 音频采样大小
    int channles;               // 声道数

    bool canSeekToEnd = 0;      // 是否可以跳转到结尾

    std::string creationdate;   // 创建日期
    int duration = 0;           // 时长
    int64_t filesize = 0;       // 文件大小
    double framerate = 0;       // 帧率
    int height = 0;             // 高度
    bool stereo = true;         // 是否立体声

    int videocodecid = -1;      // 视频编码ID
    int64_t videodatarate = 0;  // 视频码率
    int width = 0;              // 宽度
    int64_t pts = 0;           // 时间戳
};

// 音频原始数据消息类
class AudioRawMsg : public MsgBaseObj {
public:
    AudioRawMsg(int size, int with_adts = 0){
        this->size = size;
        type = 0;
        with_adts_ = with_adts;
        data = (unsigned char*)malloc(size*sizeof(char));
    }
    AudioRawMsg(const unsigned char*buf,int bufLen, int with_adts = 0){
        this->size = bufLen;
        type = buf[4] & 0x1f;
        with_adts_ = with_adts;
        data = (unsigned char*)malloc(bufLen*sizeof(char));
        memcpy(data,buf,bufLen);
    }
    virtual ~AudioRawMsg(){
        if(data)
            free(data);
    }

    int type;                       // 音频类型
    int size;                       // 数据大小
    int with_adts_ = 0;              // 是否包含ADTS头
    unsigned char *data = NULL;      // 音频数据
    uint32_t pts;                   // 时间戳
};

// 音频规格消息类
class AudioSpecMsg : public MsgBaseObj {
public:
    AudioSpecMsg(uint8_t profile, uint8_t channel_num, uint32_t samplerate){
        profile_ = profile;
        channels_ = channel_num;
        sample_rate_ = samplerate;
    }
    virtual ~AudioSpecMsg(){}

    uint8_t profile_ = 2;        // 音频配置文件(2:AAC LC)
    uint8_t channels_ = 2;       // 声道数
    uint32_t sample_rate_ = 48000; // 采样率
    int64_t pts_;               // 时间戳
};

// NAL单元结构
class NaluStruct : public MsgBaseObj {
public:
    NaluStruct(int size){
        this->size = size;
        type = 0;
        data = (unsigned char*)malloc(size*sizeof(char));
    }
    NaluStruct(const unsigned char*buf, int bufLen){
        this->size = bufLen;
        type = buf[4] & 0x1f;
        data = (unsigned char*)malloc(bufLen*sizeof(char));
        memcpy(data,buf,bufLen);
    }
    virtual ~NaluStruct(){
        if(data)
        {
            free(data);
            data = NULL;
        }
    }

    int type;                   // NAL类型
    int size;                   // 数据大小
    unsigned char *data = NULL; // NAL数据
    uint32_t pts;              // 时间戳
};

// 视频序列头消息类
class VideoSequenceHeaderMsg : public MsgBaseObj {
public:
    VideoSequenceHeaderMsg(uint8_t *sps, int sps_size, uint8_t* pps, int pps_size){
        sps_ = (uint8_t *)malloc(sps_size*sizeof(uint8_t));
        pps_ = (uint8_t *)malloc(pps_size*sizeof(uint8_t));
        if(!sps_ || !pps_)
        {

            LOG(ERROR)<<"VideoSequenceHeaderMsg malloc failed";
            return;
        }
        sps_size_ = sps_size;
        memcpy(sps_, sps, sps_size);
        pps_size_ = pps_size;
        memcpy(pps_, pps, pps_size);
    }
    virtual ~VideoSequenceHeaderMsg(){
        if(sps_)
            free(sps_);
        if(pps_)
            free(pps_);
    }

    uint8_t* sps_;             // SPS数据
    int sps_size_;             // SPS大小
    uint8_t* pps_;             // PPS数据
    int pps_size_;             // PPS大小
    unsigned int nWidth;       // 视频宽度
    unsigned int nHeight;      // 视频高度
    unsigned int nFrameRate;   // 帧率
    unsigned int nVideoDataRate; // 视频码率
    int64_t pts_ = 0;         // 时间戳
};

// RTMP数据包消息类
class MsgRTMPPPack : MsgBaseObj
{
public:
    RTMPPacket *rtmpPack = NULL;// RTMP包指针
    MsgRTMPPPack(RTMPPacket& pack)
    {
        rtmpPack = (RTMPPacket *)malloc(sizeof(RTMPPacket));
        memcpy(rtmpPack,&pack,sizeof(RTMPPacket));
    }
    virtual ~MsgRTMPPPack()
    {
        if(rtmpPack)
        {
            RTMPPacket_Free(rtmpPack);
            rtmpPack = NULL;
        }
    }
};

// 用来debug rtmp拉流的关键时间点
class AVPlayTime
{
public:
    static AVPlayTime* GetInstance() {
        static AVPlayTime s_play_time;
        return &s_play_time;
    }

    AVPlayTime() {
        start_time_ = getCurrentTimeMsec();
    }

    void Rest() {
        start_time_ = getCurrentTimeMsec();
    }
    // 各个关键点的时间戳
    inline const char *getKeyTimeTag() {
        return "keytime";
    }
    // rtmp位置关键点
    inline const char *getRtmpTag() {
        return "keytime:rtmp_pull";
    }
    // 获取到metadata
    inline const char *getMetadataTag() {
        return "metadata";
    }
    // aac sequence header
    inline const char *getAacHeaderTag() {
        return "aacheader";
    }
    // aac raw data
    inline const char *getAacDataTag() {
        return "aacdata";
    }
    // avc sequence header
    inline const char *getAvcHeaderTag() {
        return "avcheader";
    }

    // 第一个i帧
    inline const char *getAvcIFrameTag() {
        return "avciframe";
    }
    // 第一个非i帧
    inline const char *getAvcFrameTag() {
        return "avcframe";
    }
    // 音视频解码
    inline const char *getAcodecTag() {
        return "keytime:acodec";
    }
    inline const char *getVcodecTag() {
        return "keytime:vcodec";
    }
    // 音视频输出
    inline const char *getAoutTag() {
        return "keytime:aout";
    }
    inline const char *getVoutTag() {
        return "keytime:vout";
    }

    // 返回毫秒
    uint32_t getCurrenTime() {
        int64_t t = getCurrentTimeMsec() - start_time_;

        return (uint32_t)(t%0xffffffff);

    }

private:
    int64_t getCurrentTimeMsec() {
#ifdef _WIN32
        struct timeval tv;
        time_t clock;
        struct tm tm;
        SYSTEMTIME wtm;
        GetLocalTime(&wtm);
        tm.tm_year = wtm.wYear - 1900;
        tm.tm_mon = wtm.wMonth - 1;
        tm.tm_mday = wtm.wDay;
        tm.tm_hour = wtm.wHour;
        tm.tm_min = wtm.wMinute;
        tm.tm_sec = wtm.wSecond;
        tm.tm_isdst = -1;
        clock = mktime(&tm);
        tv.tv_sec = clock;
        tv.tv_usec = wtm.wMilliseconds * 1000;
        return ((unsigned long long)tv.tv_sec * 1000 + ( long)tv.tv_usec / 1000);
#else
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return ((unsigned long long)tv.tv_sec * 1000 + (long)tv.tv_usec / 1000);
#endif
    }

    int64_t start_time_ = 0;

    static AVPlayTime * s_play_time;
};

#endif // MEDIABASE_H
