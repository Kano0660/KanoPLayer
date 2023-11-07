#include "av_decoder.h"
#include <QDebug>
#include "threadpool.h"
#include <QThread>

using AVTool::Decoder;

Decoder::Decoder()
    : m_maxFrameQueueSize(16),
      m_maxPacketQueueSize(30),
      m_videoIndex(-1),
      m_audioIndex(-1),
      m_duration(0),
      m_fmtCtx(nullptr),
      m_exit(0)
{
    if(!init())
        qDebug()<<"Decoder init failed!\n";    
}

Decoder::~Decoder()
{
    exit();
}

bool Decoder::init()
{
    if(!ThreadPool::init()) {
        qDebug()<<"threadpool init failed!\n";
        return false;
    }

    m_audioPacketQueue.pktVec.resize(m_maxPacketQueueSize);
    m_videoPacketQueue.pktVec.resize(m_maxPacketQueueSize);

    m_audioFrameQueue.frameVec.resize(m_maxFrameQueueSize);
    m_videoFrameQueue.frameVec.resize(m_maxFrameQueueSize);

    m_audioPktDecoder.codecCtx=nullptr;
    m_videoPktDecoder.codecCtx=nullptr;

    return true;
}



void Decoder::setInitVal()
{
    m_audioPacketQueue.size=0;
    m_audioPacketQueue.pushIndex=0;
    m_audioPacketQueue.readIndex=0;
    m_audioPacketQueue.serial=0;

    m_videoPacketQueue.size=0;
    m_videoPacketQueue.pushIndex=0;
    m_videoPacketQueue.readIndex=0;
    m_videoPacketQueue.serial=0;

    m_audioFrameQueue.size=0;
    m_audioFrameQueue.readIndex=0;
    m_audioFrameQueue.pushIndex=0;
    m_audioFrameQueue.shown=0;

    m_videoFrameQueue.size=0;
    m_videoFrameQueue.readIndex=0;
    m_videoFrameQueue.pushIndex=0;
    m_videoFrameQueue.shown=0;

    m_exit = 0;

    m_isSeek=0;

    m_audSeek=0;
    m_vidSeek=0;

    m_audioPktDecoder.serial=0;
    m_videoPktDecoder.serial=0;
}

int Decoder::decode(const QString& url)
{
    int ret = 0;
    //解封装初始化
    m_fmtCtx = avformat_alloc_context();

    //用于获取流时长
    AVDictionary* formatOpts=nullptr;
    av_dict_set(&formatOpts,"probesize","32",0);//av_dict_set()函数用于向字典中添加键值对，key,value,flags

    // 打开媒体文件
    ret = avformat_open_input(&m_fmtCtx, url.toUtf8().constData(), nullptr, nullptr);
    //avformat_open_input
    //根据传入的url确定了要使用的协议URLProtocol，比如http的或是file类型的协议；
    //然后按该协议打开文件或建立连接
    //如果是视频文件，会探测其封装格式并将视频源装入内部buffer中；如果是网络流视频，则会创建socket等工作连接视频获取其内容，装入内部buffer中。最后读取视频头信息。

    if (ret < 0) {
        av_strerror(ret, m_errBuf, sizeof(m_errBuf));
        qDebug() << "avformat_open_input error:" << m_errBuf;
        av_dict_free(&formatOpts);
        //打开失败释放分配的AVFormatContext内存
        avformat_free_context(m_fmtCtx);
        return 0;
    }
    // 查找所有媒体流信息
    ret = avformat_find_stream_info(m_fmtCtx, nullptr);
    if (ret < 0) {
        av_strerror(ret, m_errBuf, sizeof(m_errBuf));
        qDebug() << "avformat_find_stream_info error:" << m_errBuf;
        av_dict_free(&formatOpts);
        return 0;
    }

    //记录流时长
    //    AVRational 是 FFmpeg 中表示有理数（ratio）的数据结构，用于进行时间戳和帧率等计算。它的定义如下：
    //        typedef struct AVRational {
    //        int num; ///< Numerator (分子)
    //        int den; ///< Denominator (分母)
    //    } AVRational;

    AVRational q={1,AV_TIME_BASE};//时间基#define         AV_TIME_BASE   1000000
    m_duration=(uint32_t)(m_fmtCtx->duration*av_q2d(q));//av_q2d()将时间从 AVRational 形式转换为 double 形式
    av_dict_free(&formatOpts);//删除字典内容



    decodeAudio(std::ref(m_fmtCtx));
    decodeVedio(std::ref(m_fmtCtx));

    //记录视频帧率
    m_vidFrameRate=av_guess_frame_rate(m_fmtCtx,m_fmtCtx->streams[m_videoIndex],nullptr);

    setInitVal();

    ThreadPool::addTask(std::bind(&Decoder::demux,this,std::placeholders::_1),std::make_shared<int>(1));
    ThreadPool::addTask(std::bind(&Decoder::audioDecode,this,std::placeholders::_1),std::make_shared<int>(2));
    ThreadPool::addTask(std::bind(&Decoder::videoDecode,this,std::placeholders::_1),std::make_shared<int>(3));

    return 1;
}
int Decoder::decodeAudio(AVFormatContext* m_fmtCtx)
{
     int ret = 0;
    //av_find_best_stream函数查找最佳的视频/音频流。如果找到了最佳视频/音频流，它会获取视频流的参数信息，如分辨率和编码方式，并打印出来。
    m_audioIndex = av_find_best_stream(m_fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (m_audioIndex < 0) {
        qDebug() << "no audio stream!";
        return 0;
    }

    //音频解码初始化
    //AVCodecParameters是FFmpeg库中的一个结构体，用于保存音视频流的基本参数信息
    AVCodecParameters* audioCodecPar = m_fmtCtx->streams[m_audioIndex]->codecpar;
    if (!audioCodecPar) {
        qDebug() << "audio par is nullptr!";
        return 0;
    }


    // 获取音频解码器
    const AVCodec* audioCodec = avcodec_find_decoder(audioCodecPar->codec_id);
    if (!audioCodec) {
        qDebug() << "avcodec_find_decoder failed!";
        return 0;
    }

    // 创建音频解码器上下文
    m_audioPktDecoder.codecCtx = avcodec_alloc_context3(audioCodec);
    //配置音频解码器上下文
    ret = avcodec_parameters_to_context(m_audioPktDecoder.codecCtx, audioCodecPar);
    if (ret < 0) {
        av_strerror(ret, m_errBuf, sizeof(m_errBuf));
        qDebug() << "error info_avcodec_parameters_to_context:" << m_errBuf;
        return 0;
    }

    //打开音频流
    ret = avcodec_open2(m_audioPktDecoder.codecCtx, nullptr, nullptr);


    if (ret < 0) {
        av_strerror(ret, m_errBuf, sizeof(m_errBuf));
        qDebug() << "error info_avcodec_open2:" << m_errBuf;
        return 0;
    }

}



int Decoder::decodeVedio(AVFormatContext* m_fmtCtx)
{
     int ret = 0;

    //av_find_best_stream函数查找最佳的视频/音频流。如果找到了最佳视频/音频流，它会获取视频流的参数信息，如分辨率和编码方式，并打印出来。
    m_videoIndex = av_find_best_stream(m_fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_videoIndex < 0) {
        qDebug() << "no video stream!";
        return 0;
    }


    //视频解码初始化
    AVCodecParameters* videoCodecPar= m_fmtCtx->streams[m_videoIndex]->codecpar;
    if (!videoCodecPar) {
        qDebug() << "videocodecpar is nullptr!";
        return 0;
    }

    //获取视频解码器
    const AVCodec* videoCodec = avcodec_find_decoder(videoCodecPar->codec_id);
    if (!videoCodec) {
        qDebug() << "avcodec_find_decoder failed!";
        return 0;
    }

    // 创建视频解码器上下文
    m_videoPktDecoder.codecCtx = avcodec_alloc_context3(videoCodec);
    //配置音频解码器上下文
    ret = avcodec_parameters_to_context(m_videoPktDecoder.codecCtx, videoCodecPar);
    if (ret < 0) {
        av_strerror(ret, m_errBuf, sizeof(m_errBuf));
        qDebug() << "error info_avcodec_parameters_to_context:" << m_errBuf;
        return 0;
    }

    //打开视频流
    ret = avcodec_open2(m_videoPktDecoder.codecCtx, videoCodec, nullptr);
    if (ret < 0) {
        av_strerror(ret, m_errBuf, sizeof(m_errBuf));
        qDebug() << "error info_avcodec_open2:" << m_errBuf;
        return 0;
    }
}

void Decoder::exit()
{
    m_exit=1;
    QThread::msleep(200);
    clearQueueCache();
    if(m_fmtCtx) {
        avformat_close_input(&m_fmtCtx);
        m_fmtCtx=nullptr;
    }
    if(m_audioPktDecoder.codecCtx) {
        avcodec_free_context(&m_audioPktDecoder.codecCtx);
        m_audioPktDecoder.codecCtx=nullptr;
    }
    if(m_videoPktDecoder.codecCtx) {
        avcodec_free_context(&m_videoPktDecoder.codecCtx);
        m_videoPktDecoder.codecCtx=nullptr;
    }
}

void Decoder::clearQueueCache()
{
    std::lock_guard<std::mutex> lockAP(m_audioPacketQueue.mutex);
    std::lock_guard<std::mutex> lockVP(m_videoPacketQueue.mutex);

    while(m_audioPacketQueue.size) {      
        av_packet_unref(&m_audioPacketQueue.pktVec[m_audioPacketQueue.readIndex].pkt);
        m_audioPacketQueue.readIndex=(m_audioPacketQueue.readIndex+1)%m_maxPacketQueueSize;
        m_audioPacketQueue.size--;
    }

    while(m_videoPacketQueue.size) {
        av_packet_unref(&m_videoPacketQueue.pktVec[m_videoPacketQueue.readIndex].pkt);
        m_videoPacketQueue.readIndex=(m_videoPacketQueue.readIndex+1)%m_maxPacketQueueSize;
        m_videoPacketQueue.size--;
    }

    std::lock_guard<std::mutex> lockAF(m_audioFrameQueue.mutex);
    std::lock_guard<std::mutex> lockVF(m_videoFrameQueue.mutex);

    while(m_audioFrameQueue.size) {
        av_frame_unref(&m_audioFrameQueue.frameVec[m_audioFrameQueue.readIndex].frame);
        m_audioFrameQueue.readIndex=(m_audioFrameQueue.readIndex+1)%m_maxFrameQueueSize;
        m_audioFrameQueue.size--;
    }

    while(m_videoFrameQueue.size) {
        av_frame_unref(&m_videoFrameQueue.frameVec[m_videoFrameQueue.readIndex].frame);
        m_videoFrameQueue.readIndex=(m_videoFrameQueue.readIndex+1)%m_maxFrameQueueSize;
        m_videoFrameQueue.size--;
    }
}

void Decoder::packetQueueFlush(PacketQueue* queue)
{
    std::lock_guard<std::mutex> lockAP(queue->mutex);

    while(queue->size) {
        av_packet_unref(&queue->pktVec[queue->readIndex].pkt);//释放数据包
        queue->readIndex=(queue->readIndex+1)%m_maxPacketQueueSize;
        queue->size--;
    }
    queue->serial++;
}

void Decoder::seekTo(int32_t target,int32_t seekRel)
{
    //上次跳转未完成不处理跳转请求
    if(m_isSeek==1)
        return;
    if(target<0)
        target=0;
    m_seekTarget=target;
    m_seekRel=seekRel;
    m_isSeek = 1;
}

void Decoder::demux(std::shared_ptr<void> par)
{
    int ret = -1;
    //创建音视频数据包
    AVPacket* pkt = av_packet_alloc();
    while (1) {
        if (m_exit) {
            break;
        }
        if (m_audioPacketQueue.size >= m_maxPacketQueueSize||m_videoPacketQueue.size>=m_maxPacketQueueSize) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        if(m_isSeek) {
            //AVRational sq={1,AV_TIME_BASE};
            //int64_t seekMin = m_seekRel > 0 ? m_seekTarget-m_seekRel+2 : INT64_MIN;
            //int64_t seekMax = m_seekRel < 0 ? m_seekTarget-m_seekRel-2 : INT64_MAX;
            //qDebug()<<"seekMin:"<<seekMin<<" seekMax:"<<seekMax<<" seekTarget:"<<m_seekTarget<<endl;
            //ret=avformat_seek_file(m_fmtCtx,m_audioIndex,seekMin,m_seekTarget,seekMax,AVSEEK_FLAG_BACKWARD);

            //检索时间戳，
            int64_t seekTarget=m_seekTarget*AV_TIME_BASE;

            //想实现播放进度控制，就意味着需要随机的访问流媒体文件，那么就需要使用av_seek_frame或者avformat_seek_file函数。
            ret=av_seek_frame(m_fmtCtx,-1,seekTarget,AVSEEK_FLAG_BACKWARD);
            if(ret<0) {
                av_strerror(ret, m_errBuf, sizeof(m_errBuf));
                qDebug() << "avformat_seek_file error:" << m_errBuf;
            }
            else {
                packetQueueFlush(&m_audioPacketQueue);
                packetQueueFlush(&m_videoPacketQueue);
                m_audSeek=1;
                m_vidSeek=1;
            }
            m_isSeek=0;
        }

        ret = av_read_frame(m_fmtCtx, pkt);//读取一个包，需要说明的是此函数必须是包含整数帧的
        if (ret != 0) {
            av_packet_free(&pkt);
            // av_strerror函数可以帮我们当使用ffmepg里面的API接口时，如果发生错误，得到返回值错误码的同时，
            //还会返回一个错误码相对应的描述。对于我们分析问题十分的方便！！！！
            av_strerror(ret, m_errBuf, sizeof(m_errBuf));
            qDebug() << "av_read_frame error:" << m_errBuf;
            break;
        }
        if (pkt->stream_index == m_audioIndex) {
            //插入音频包队列
            //qDebug()<<pkt->pts*av_q2d(m_fmtCtx->streams[m_audioIndex]->time_base)<<endl;
            pushPacket(&m_audioPacketQueue,pkt);
        }
        else if (pkt->stream_index == m_videoIndex) {
            //插入视频包队列
            pushPacket(&m_videoPacketQueue,pkt);
            //av_packet_unref(pkt);
        }
        else {
            av_packet_unref(pkt);
        }
    }
    av_packet_free(&pkt);
    //是读到文件末尾退出的才清空，强制退出不重复此操作
    if(!m_exit) {
        while(m_audioFrameQueue.size)
            QThread::msleep(50);
        exit();
    }
    qDebug() << "demuxthread exit";
}

void Decoder::audioDecode(std::shared_ptr<void> par)
{
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    while (1)
    {
        if (m_exit) {
            break;
        }
        //音频帧队列长度控制
        if (m_audioFrameQueue.size >= m_maxFrameQueueSize) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        //从音频包队列取音频包
        int ret=getPacket(&m_audioPacketQueue,pkt,&m_audioPktDecoder);
        if (ret) {
            // 送入要解码的数据包
            ret = avcodec_send_packet(m_audioPktDecoder.codecCtx, pkt);
            av_packet_unref(pkt);
            if (ret < 0) { // 送入输入数据包失败
                av_strerror(ret, m_errBuf, sizeof(m_errBuf));
                qDebug() << "avcodec_send_packet error:" << m_errBuf;
                continue;
            }
            while(1) {
                // 获取解码后的视频帧或者音频帧
                ret = avcodec_receive_frame(m_audioPktDecoder.codecCtx, frame);
                if (ret == 0) {
                    if(m_audSeek) {
                        int pts=(int)frame->pts*av_q2d(m_fmtCtx->streams[m_audioIndex]->time_base);
                        //qDebug()<<"audFrame pts:"<<pts<<endl;
                        if(pts<m_seekTarget) {//跳转的时刻与当前帧的pts进行对比,如果当前帧在跳转时刻之前应当丢弃
                            av_frame_unref(frame);
                            continue;
                        }
                        else {
                            m_audSeek=0;
                        }
                    }
                   //添加到待播放音频帧队列
                    pushAFrame(frame);
                }
                else {
                    break;
                }
            }
        }
        else {
            //qDebug() << "audio packetQueue is empty for decoding!";
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    av_packet_free(&pkt);
    av_frame_free(&frame); 
    qDebug() << "audioDecode exit";
}

void Decoder::videoDecode(std::shared_ptr<void> par)
{
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    while (1)
    {
        if (m_exit) {
            break;
        }
        if (m_videoFrameQueue.size >=m_maxFrameQueueSize) {	//视频帧队列长度控制
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        int ret=getPacket(&m_videoPacketQueue,pkt,&m_videoPktDecoder);
        if (ret) {
            ret = avcodec_send_packet(m_videoPktDecoder.codecCtx, pkt);
            av_packet_unref(pkt);
            if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_strerror(ret, m_errBuf, sizeof(m_errBuf));
                qDebug() << "avcodec_send_packet error:" << m_errBuf;
                continue;
            }      
            while(1) {
                ret = avcodec_receive_frame(m_videoPktDecoder.codecCtx, frame);
                if (ret == 0) {
                    if(m_vidSeek) {
                        int pts=(int)frame->pts*av_q2d(m_fmtCtx->streams[m_videoIndex]->time_base);
                        if(pts<m_seekTarget) {
                            av_frame_unref(frame);
                            continue;
                        }
                        else {
                            m_vidSeek=0;
                        }
                    }
                    //AVRational vidRational=av_guess_frame_rate(m_fmtCtx,
                    //              m_fmtCtx->streams[m_videoIndex],frame);
                   //添加到待播放视频帧队列
                    pushVFrame(frame);
                }
                else {
                    break;
                }
            }
        }
        else {
            //qDebug() << "video packetQueue is empty for decoding!";
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    av_packet_free(&pkt);
    av_frame_free(&frame);
    qDebug() << "videoDecode exit";
}

int Decoder::getPacket(PacketQueue* queue,AVPacket* pkt,PktDecoder* decoder)
{
    std::unique_lock<std::mutex> lock(queue->mutex);
    while(!queue->size) { 
        bool ret=queue->cond.wait_for(lock,std::chrono::milliseconds(100),
                                      [&](){return queue->size&!m_exit;});
        if(!ret)
            return 0;
    }
    if(queue->serial!=decoder->serial) {
        //序列号不连续的帧证明发生了跳转操作则直接丢弃
        //并清空解码器缓存
        avcodec_flush_buffers(decoder->codecCtx);
        decoder->serial=queue->pktVec[queue->readIndex].serial;
        return 0;
    }
    av_packet_move_ref(pkt,&queue->pktVec[queue->readIndex].pkt);
    decoder->serial=queue->pktVec[queue->readIndex].serial;
    queue->readIndex=(queue->readIndex+1)%m_maxPacketQueueSize;
    queue->size--;
    return 1;
}

void Decoder::pushPacket(PacketQueue* queue,AVPacket* pkt)
{
    std::lock_guard<std::mutex> lock(queue->mutex);
    av_packet_move_ref(&queue->pktVec[queue->pushIndex].pkt,pkt);
    queue->pktVec[queue->pushIndex].serial=queue->serial;
    if(queue->serial==1)
        qDebug()<<"";
    queue->pushIndex=(queue->pushIndex+1)%m_maxPacketQueueSize;
    queue->size++;
}

void Decoder::pushAFrame(AVFrame* frame)
{
    std::lock_guard<std::mutex> lock(m_audioFrameQueue.mutex);
    av_frame_move_ref(&m_audioFrameQueue.frameVec[m_audioFrameQueue.pushIndex].frame,frame);
    m_audioFrameQueue.frameVec[m_audioFrameQueue.pushIndex].serial=m_audioPktDecoder.serial;
    m_audioFrameQueue.pushIndex=(m_audioFrameQueue.pushIndex+1)%m_maxFrameQueueSize;
    m_audioFrameQueue.size++;
}

int Decoder::getAFrame(AVFrame* frame)
{
    if(!frame)
        return 0;
    std::unique_lock<std::mutex> lock(m_audioFrameQueue.mutex);
    while(!m_audioFrameQueue.size) {
        bool ret=m_audioFrameQueue.cond.wait_for(lock,std::chrono::milliseconds(100),
                                                 [&](){return !m_exit&m_audioFrameQueue.size;});
        if(!ret)
            return 0;
    }  
    if(m_audioFrameQueue.frameVec[m_audioFrameQueue.readIndex].serial!=m_audioPacketQueue.serial) {
        av_frame_unref(&m_audioFrameQueue.frameVec[m_audioFrameQueue.readIndex].frame);
        m_audioFrameQueue.readIndex=(m_audioFrameQueue.readIndex+1)%m_maxFrameQueueSize;
        m_audioFrameQueue.size--;
        return 0;
    }
    av_frame_move_ref(frame,&m_audioFrameQueue.frameVec[m_audioFrameQueue.readIndex].frame);
    m_audioFrameQueue.readIndex=(m_audioFrameQueue.readIndex+1)%m_maxFrameQueueSize;
    m_audioFrameQueue.size--;
    return 1;
}

void Decoder::pushVFrame(AVFrame* frame)
{
    std::lock_guard<std::mutex> lock(m_videoFrameQueue.mutex);   
    m_videoFrameQueue.frameVec[m_videoFrameQueue.pushIndex].serial=m_videoPktDecoder.serial;
    m_videoFrameQueue.frameVec[m_videoFrameQueue.pushIndex].duration=\
            (m_vidFrameRate.den&&m_vidFrameRate.den) ? av_q2d(AVRational{m_vidFrameRate.den,m_vidFrameRate.num}) : 0.00;
    m_videoFrameQueue.frameVec[m_videoFrameQueue.pushIndex].pts=\
            frame->pts*av_q2d(m_fmtCtx->streams[m_videoIndex]->time_base);
    av_frame_move_ref(&m_videoFrameQueue.frameVec[m_videoFrameQueue.pushIndex].frame,frame);
    m_videoFrameQueue.pushIndex=(m_videoFrameQueue.pushIndex+1)%m_maxFrameQueueSize;
    m_videoFrameQueue.size++;
    //qDebug()<<"RemainingVFrame:"<<m_videoFrameQueue.size-m_videoFrameQueue.shown;
}

Decoder::MyFrame* Decoder::peekLastVFrame()
{
    Decoder::MyFrame* frame=&m_videoFrameQueue.frameVec[m_videoFrameQueue.readIndex];
    return frame;
}

Decoder::MyFrame* Decoder::peekVFrame()
{
    while(!m_videoFrameQueue.size) {
        std::unique_lock<std::mutex> lock(m_videoFrameQueue.mutex);
        bool ret=m_videoFrameQueue.cond.wait_for(lock,std::chrono::milliseconds(100),
                                                 [&](){return !m_exit&m_videoFrameQueue.size;});
        if(!ret)
            return nullptr;
    }
    int index=(m_videoFrameQueue.readIndex+m_videoFrameQueue.shown)%m_maxFrameQueueSize;
    Decoder::MyFrame* frame=&m_videoFrameQueue.frameVec[index];
    return frame;
}

Decoder::MyFrame* Decoder::peekNextVFrame()
{
    while(m_videoFrameQueue.size<2) {
        std::unique_lock<std::mutex> lock(m_videoFrameQueue.mutex);
        bool ret=m_videoFrameQueue.cond.wait_for(lock,std::chrono::milliseconds(100),
                                                 [&](){return !m_exit&m_videoFrameQueue.size;});
        if(!ret)
            return nullptr;
    }
    int index=(m_videoFrameQueue.readIndex+m_videoFrameQueue.shown+1)%m_maxFrameQueueSize;
    Decoder::MyFrame* frame=&m_videoFrameQueue.frameVec[index];
    return frame;
}

void Decoder::setNextVFrame()
{
    std::unique_lock<std::mutex> lock(m_videoFrameQueue.mutex);
    if(!m_videoFrameQueue.size)
        return;
    if(!m_videoFrameQueue.shown) {
        m_videoFrameQueue.shown=1;
        return;
    }
    av_frame_unref(&m_videoFrameQueue.frameVec[m_videoFrameQueue.readIndex].frame);
    m_videoFrameQueue.readIndex=(m_videoFrameQueue.readIndex+1)%m_maxFrameQueueSize; 
    m_videoFrameQueue.size--;
}

int Decoder::getRemainingVFrame()
{
    if(!m_videoFrameQueue.size)
        return 0;
    return m_videoFrameQueue.size-m_videoFrameQueue.shown;
}

