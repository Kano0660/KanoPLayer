#include "av_player.h"
#include <QDebug>
#include <QImage>
#include "threadpool.h"
#include "vframe.h"
#include <QThread>
#include <iostream>

//同步阈值下限
#define AV_SYNC_THRESHOLD_MIN 0.04
//同步阈值上限
#define AV_SYNC_THRESHOLD_MAX 0.1
//单帧视频时长阈值上限，用于适配低帧时同步，
//帧率过低视频帧超前不适合翻倍延迟，应特殊
//处理，这里设置上限一秒10帧
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
//同步操作摆烂阈值上限，此时同步已无意义
#define AV_NOSYNC_THRESHOLD 10.0

#define AV_SYNC_REJUDGESHOLD 0.01

AVPlayer::AVPlayer()
    : m_decoder(new Decoder),
      m_audioFrame(av_frame_alloc()),
      m_imageWidth(300),
      m_imageHeight(300),
      m_swrCtx(nullptr),
      m_swsCtx(nullptr),
      m_buffer(nullptr),
      m_audioBuf(nullptr),
      m_duration(0),
      m_volume(30),
      m_exit(0),
      m_pause(0)
{
}

AVPlayer::~AVPlayer()
{
    av_frame_free(&m_audioFrame);
    clearPlayer();
    delete m_decoder;
    if(m_swrCtx)
        swr_free(&m_swrCtx);
    if(m_swsCtx)
        sws_freeContext(m_swsCtx);
    if(m_audioBuf)
        av_free(m_audioBuf);
    if(m_buffer)
        av_free(m_buffer);
}

int AVPlayer::play(const QString& url)
{
    clearPlayer();
    if(!m_decoder->decode(url)) {
        qDebug()<<"decode failed";
        return 0;
    }

    //解码成功可获取流时长
    m_duration=m_decoder->avDuration();
    emit AVDurationChanged(m_duration);

    m_pause=0;
    m_clockInitFlag=-1;

    if(!initSDL()) {//播放音频
        qDebug()<<"init sdl failed!";
        return 0;
    }
    initVideo();//播放视频
}


void AVPlayer::soundTouuch(uint8_t*inPcmBuf,int inPcmBufsize,uint channels,uint sampleRate,int bytePerSample,double tempo,double pitch){

    // 1. 设置SoundTouch，配置变调变速参数
    s_touch = new soundtouch::SoundTouch();
    s_touch->setSampleRate(sampleRate); // 设置采样率
    s_touch->setChannels(channels); // 设置通道数
    s_touch->setRate(tempo); //变速
    s_touch->setPitch(pitch);//变调

    //计算此次数据包含的样本个数
    uint nSamples=inPcmBufsize/channels/bytePerSample;

//    // 将解码后的inPcmBuf(uint8*)转换为soundtouch::SAMPLETYPE，也就是singed int 16
//    unsigned int len =inPcmBufsize/2;
//    av_fast_malloc(&touch_buffer, &len, len+256);
//    for (auto i = 0; i < len/2+1; i++)
//    {
//        touch_buffer[i] = (inPcmBuf[i * 2] | (inPcmBuf[i * 2 + 1] << 8));
//    }

    //首先计算缓存中的字节数，然后按照小端的方式组合为16为有符号整数。然后将转换后的buffer传送给SoundTouch即可。
    s_touch->putSamples((short *)inPcmBuf, nSamples);//内部需要缓冲处理，可能put多次才能收到缓冲数据
    while(1)
    {
        //返回缓存区处理好的样本个数，单次个数不超过96000
        nSamples = s_touch->receiveSamples(touch_buffer, 96000);
        if(nSamples == 0)
            break;

        //计算处理后的数据长度，变速返回的样本数会增大或减少，由调用方判断需读取的处理长度
        int length =  nSamples * channels * av_get_bytes_per_sample((enum AVSampleFormat)bytePerSample);
        //写入处理后的缓冲区
        memcpy(dataBuf+bufPos,(uint8_t *)touch_buffer, length);
        bufPos += length;
    }


    //缓冲区，通道数，采样率,帧格式(enum AVSampleFormat),变速，变调参数
    //    变调不变速
    //        setPitch(double newPitch)源pitch = 1.0，小于1音调变低；大于1音调变高
    //        setPitchOctaves(double newPitch) 在源pitch的基础上，使用八度音(Octave)设置新的pitch [-1.00, 1.00]。
    //        setPitchSemiTones(double or int newPitch) 在源pitch的基础上，使用半音(Semitones)设置新的pitch [-12.0,12.0]

    //    变速不变调

    //        setRate(double newRate) 设置新的rate，源rate=1.0，小于1变慢；大于1变快
    //        setRateChange(double newRate) 在源rate的基础上，以百分比设置新的rate[-50,100]
    //        setTempo(double newTempo) 设置新的节拍tempo，源tempo=1.0，小于1则变慢；大于1变快
    //        setTempoChange(double newTempo) 在源tempo的基础上，以百分比设置新的tempo[-50,100]

}


void fillAStreamCallback(void* userdata, uint8_t* stream,int len)
{
    memset(stream, 0, len);
    AVPlayer* is = (AVPlayer*)userdata;
    double audioPts=0.00;
    while (len > 0) {
        if(is->m_exit)
            return;
        if (is->m_audioBufIndex >= is->m_audioBufSize) { /*index到缓冲区末尾,重新填充数据*/

            int ret= is->m_decoder->getAFrame(is->m_audioFrame);



            if (ret) {
                is->m_audioBufIndex = 0;
                if ((is->m_targetSampleFmt != is->m_audioFrame->format ||
                     is->m_targetChannelLayout != is->m_audioFrame->channel_layout ||
                     is->m_targetFreq != is->m_audioFrame->sample_rate||
                     is->m_targetNbSamples!=is->m_audioFrame->nb_samples) &&
                    !is->m_swrCtx) {
                    is->m_swrCtx = swr_alloc_set_opts(nullptr,
                                                      is->m_targetChannelLayout, is->m_targetSampleFmt, is->m_targetFreq*is->playRate,
                                                      is->m_audioFrame->channel_layout, (enum AVSampleFormat)is->m_audioFrame->format, is->m_audioFrame->sample_rate,
                                                      0, nullptr);
                    if (!is->m_swrCtx || swr_init(is->m_swrCtx) < 0) {
                        qDebug() << "swr_init failed";
                        return;
                    }
                }
                if (is->m_swrCtx) {

                    //重采样

                    is->dataBufSize = av_samples_get_buffer_size(nullptr, is->m_targetChannels, is->m_audioFrame->nb_samples, (enum AVSampleFormat)is->m_audioFrame->format, 0);
                    av_fast_malloc(&is->dataBuf, &is->dataBufSize, is->dataBufSize+256);
                    if (!is->dataBuf) {
                        qDebug() << "av_fast_malloc failed";
                        return;
                    }
                    memcpy(is->dataBuf,is->m_audioFrame->data[0] /*is->m_audioFrame->data[0]*/, is->dataBufSize);

                    is->soundTouuch(is->dataBuf,
                                    is->dataBufSize,
                                    is->m_targetChannels,
                                    is->m_audioFrame->sample_rate,
                                    is->m_audioFrame->format,
                                    is->playRate,1);
                    //变速
                    is->dataBufSize=is->dataBufSize/is->playRate;
                    const uint8_t** in ;
                    if(is->bufPos>=is->dataBufSize){
                        uint8_t * buffer= is->dataBuf;
                        //移动位置
                        is->bufPos-=is->dataBufSize;
                        memmove(buffer,buffer+is->dataBufSize,is->bufPos);
                        in = (const uint8_t **)(&buffer);
                    }
                    else{
                        in = (const uint8_t**)is->m_audioFrame->extended_data;
                    }



                    //const uint8_t** in = (const uint8_t**)is->m_audioFrame->extended_data;

                    int out_count = (uint64_t)is->m_audioFrame->nb_samples * is->m_targetFreq / is->m_audioFrame->sample_rate + 256;
                    int out_size = av_samples_get_buffer_size(nullptr, is->m_targetChannels, out_count, is->m_targetSampleFmt, 0);
                    if (out_size < 0) {
                        qDebug() << "av_samples_get_buffer_size failed";
                        return;
                    }
                    av_fast_malloc(&is->m_audioBuf, &is->m_audioBufSize, out_size);
                    if (!is->m_audioBuf) {
                        qDebug() << "av_fast_malloc failed";
                        return;
                    }
                    //音频格式转换
                    int len2 = swr_convert(is->m_swrCtx, &is->m_audioBuf, out_count, in, is->m_audioFrame->nb_samples);
                    if (len2 < 0) {
                        qDebug() << "swr_convert failed";
                        return;
                    }
                    is->m_audioBufSize = av_samples_get_buffer_size(nullptr, is->m_targetChannels, len2, is->m_targetSampleFmt, 0);
                }
                else {
                    is->m_audioBufSize = av_samples_get_buffer_size(nullptr, is->m_targetChannels, is->m_audioFrame->nb_samples, is->m_targetSampleFmt, 0);
                    av_fast_malloc(&is->m_audioBuf, &is->m_audioBufSize, is->m_audioBufSize+256);
                    if (!is->m_audioBuf) {
                        qDebug() << "av_fast_malloc failed";
                        return;
                    }
                    memcpy(is->m_audioBuf, is->m_audioFrame->data[0], is->m_audioBufSize);
                }
                audioPts = is->m_audioFrame->pts * av_q2d(is->m_fmtCtx->streams[is->m_audioIndex]->time_base);
                //qDebug()<<is->m_audioPts;
                av_frame_unref(is->m_audioFrame);
            }
            else {
                //判断是否真正播放到文件末尾
                if(is->m_decoder->isExit()) {
                    emit is->AVTerminate();
                }
                return;
            }
        }
        int len1 = is->m_audioBufSize - is->m_audioBufIndex;
        len1 = (len1 > len ? len : len1);
        SDL_MixAudio(stream, is->m_audioBuf + is->m_audioBufIndex, len1,is->m_volume);
        len -= len1;
        is->m_audioBufIndex += len1;
        stream += len1;
    }
    //记录音频时钟
    is->m_audioClock.setClock(audioPts);
    //发送时间戳变化信号,因为进度以整数秒单位变化展示，
    //所以大于一秒才发送，避免过于频繁的信号槽通信消耗性能
    uint32_t _pts=(uint32_t)audioPts;
    if(is->m_lastAudPts!=_pts) {
        emit is->AVPtsChanged(_pts);
        is->m_lastAudPts=_pts;
    }
}



int AVPlayer::initSDL()//音频帧
{

    if (SDL_Init(SDL_INIT_AUDIO)!=0) {
        qDebug() << "SDL_Init failed";
        return 0;
    }

    m_exit=0;

    m_audioBufSize=0;
    m_audioBufIndex=0;

    m_lastAudPts=-1;

    m_audioCodecPar=m_decoder->audioCodecPar();

    SDL_AudioSpec wanted_spec;
    wanted_spec.channels = m_audioCodecPar->channels;
    wanted_spec.freq = m_audioCodecPar->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.callback = fillAStreamCallback;
    wanted_spec.userdata = this;
    wanted_spec.samples = m_audioCodecPar->frame_size;

    if (SDL_OpenAudio(&wanted_spec, nullptr) < 0) {
        qDebug() << "SDL_OpenAudio failed";
        return 0;
    }
    m_targetSampleFmt = AV_SAMPLE_FMT_S16;
    m_targetChannels = m_audioCodecPar->channels;
    m_targetFreq = m_audioCodecPar->sample_rate;
    m_targetChannelLayout = av_get_default_channel_layout(m_targetChannels);
    m_targetNbSamples = m_audioCodecPar->frame_size;
    m_audioIndex = m_decoder->audioIndex();
    m_fmtCtx = m_decoder->formatContext();

    SDL_PauseAudio(0);
    return 1;
}

int AVPlayer::initVideo()
{
    m_frameTimer=0.00;

    m_videoCodecPar=m_decoder->videoCodecPar();
    m_videoIndex=m_decoder->videoIndex();

    m_imageWidth=m_videoCodecPar->width;
    m_imageHeight=m_videoCodecPar->height;

    m_dstPixFmt = AV_PIX_FMT_YUV422P;
    m_swsFlags=SWS_BICUBIC;

    //分配存储转换后帧数据的buffer内存
    int bufSize = av_image_get_buffer_size(m_dstPixFmt, m_imageWidth, m_imageHeight, 1);
    m_buffer=(uint8_t*)av_realloc(m_buffer,bufSize * sizeof(uint8_t));
    //将buffer内存和m_pixels(存储frame数据)绑定
    av_image_fill_arrays(m_pixels,m_pitch,m_buffer, m_dstPixFmt,
                                                m_imageWidth, m_imageHeight,1);
    // 将 Y & U & V 数据分别填充到 (m_pixels[0] (m_pixels[1] (m_pixels[2] 中

    //视频帧播放 回调递插入 线程池任务队列
    if(!ThreadPool::addTask(std::bind(&AVPlayer::videoCallback,this,std::placeholders::_1),std::make_shared<int>(0))) {
        qDebug()<<"videoCallback add task failed!";
    }
    return 1;
}

void AVPlayer::pause(bool isPause)
{
    if(SDL_GetAudioStatus()==SDL_AUDIO_STOPPED)
        return;
    if(isPause) {
        if(SDL_GetAudioStatus()==SDL_AUDIO_PLAYING) {
            SDL_PauseAudio(1);
            m_pauseTime=av_gettime_relative()/1000000.0;
            m_pause=1;
        }
    }
    else {
        if(SDL_GetAudioStatus()==SDL_AUDIO_PAUSED) {
            SDL_PauseAudio(0);
            m_frameTimer+=av_gettime_relative()/1000000.0-m_pauseTime;
            m_pause=0;
        }
    }
}

void AVPlayer::clearPlayer()
{
    if(playState()!=AV_STOPPED) {
        m_exit=1;
        if(playState()==AV_PLAYING)
            SDL_PauseAudio(1);
        m_decoder->exit();
        SDL_CloseAudio();
        if(m_swrCtx)
            swr_free(&m_swrCtx);//libswscale库中的API :释放音频转换器
        if(m_swsCtx)
            sws_freeContext(m_swsCtx);//libswscale库中的API :释放视频转换器
        m_swrCtx=nullptr;
        m_swsCtx=nullptr;
    }
}

AVPlayer::PlayState AVPlayer::playState()
{
    AVPlayer::PlayState state;
    switch (SDL_GetAudioStatus()) {
        case SDL_AUDIO_PLAYING:
            state=AVPlayer::AV_PLAYING;
            break;
        case SDL_AUDIO_PAUSED:
            state=AVPlayer::AV_PAUSED;
            break;
        case SDL_AUDIO_STOPPED:
            state=AVPlayer::AV_STOPPED;
            break;
        default:
        break;
    }
    return state;
}

void AVPlayer::initAVClock()
{
    m_audioClock.setClock(0.00);
    m_videoClock.setClock(0.00);
    m_clockInitFlag=1;
}

void AVPlayer::on_playrate_changed(float speed)
{
    playRate = 1;
    playRate = playRate/speed;

}

void AVPlayer::displayImage(AVFrame* frame)
{
    if (frame) {
        //判断若是否需要格式转换
        if ((m_videoCodecPar->width != m_imageWidth ||
              m_videoCodecPar->height != m_imageHeight ||
              m_videoCodecPar->format != m_dstPixFmt)&&!m_swsCtx) {
            m_swsCtx = sws_getCachedContext(m_swsCtx,frame->width,frame->height,
                (enum AVPixelFormat)frame->format, m_imageWidth,m_imageHeight,m_dstPixFmt,
                                                              m_swsFlags, nullptr, nullptr, nullptr);
        }
        if (m_swsCtx) {
            sws_scale(m_swsCtx, frame->data, frame->linesize, 0,
                                frame->height, m_pixels, m_pitch);
            emit frameChanged(QSharedPointer<YUV420Frame>::create(m_pixels[0],m_imageWidth,m_imageHeight));
        }
        else {
            emit frameChanged(QSharedPointer<YUV420Frame>::create((uint8_t*)frame->data[0],m_imageWidth,m_imageHeight));
        }

        //记录视频时钟,将要显示帧pts
        m_videoClock.setClock(frame->pts*playRate * av_q2d(m_fmtCtx->streams[m_videoIndex]->time_base));
    }
}

void AVPlayer::videoCallback(std::shared_ptr<void> par)
{
    double time=0.00;
    double duration=0.00;
    double delay=0.00;
    if(m_clockInitFlag==-1) {
        initAVClock();
    }
    do {
        if(m_exit)
            break;
        if(m_pause) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        if(m_decoder->getRemainingVFrame()) {
            MyFrame* lastFrame=m_decoder->peekLastVFrame();//正在显示帧
            MyFrame* frame=m_decoder->peekVFrame();//将要显示帧

            //qDebug()<<"video pts:"<<frame->pts;

            if(frame->serial!=m_decoder->vidPktSerial()) {//视频数据包和视频帧序列不对应，判断视频帧队列中的下一个视频帧的序列号是否和解码器序列号相同
                m_decoder->setNextVFrame();//将读索引后移一位
                continue;
            }

            if(frame->serial!=lastFrame->serial)
                m_frameTimer=av_gettime_relative()/1000000.0;

            duration=vpDuration(lastFrame,frame);//正在显示帧的显示时长
            duration=duration*playRate;
            delay=computeTargetDelay(duration);//音视频同步延迟

            time=av_gettime_relative()/1000000.0;

            //qDebug()<<"delay:"<<delay<<endl;

            //显示时长未到
            if(time<m_frameTimer+delay) {
                QThread::msleep((uint32_t)(FFMIN(AV_SYNC_REJUDGESHOLD,m_frameTimer+delay-time)*1000));
                continue;
            }

            m_frameTimer+=delay;
            if(time-m_frameTimer>AV_SYNC_THRESHOLD_MAX)
                m_frameTimer=time;

            //队列中未显示帧一帧以上执行逻辑丢帧判断
            if(m_decoder->getRemainingVFrame()>1) {
                MyFrame* nextFrame=m_decoder->peekNextVFrame();
                duration=(nextFrame->pts-frame->pts)*playRate;
                //若主时钟超前到大于当前帧理论显示应持续的时间了，则当前帧立即丢弃
                if(time>m_frameTimer+duration) {
                    m_decoder->setNextVFrame();
                    qDebug()<<"abandon vframe";
                    continue;
                }
            }

            displayImage(&frame->frame);
            //读索引后移
            m_decoder->setNextVFrame();
        }
        else {
            QThread::msleep(10);
        }
    }while(true);
    qDebug()<<"videoCallBack exit";
}


double AVPlayer::computeTargetDelay(double delay)
{

    //例如在0时刻同时播放了第1帧视频和第1帧音频，根据第2帧音/视频帧pts，存在音频时钟X,视频时钟Y
    //时钟X表示经过X的时间应当播放第2帧音频  时钟Y表示经过Y的视频播放第2帧视频
    //第2帧音频到点(X时间段)会立即播放，视频如何与音频同步?
    //第2帧视频播放时刻 = 第1帧视频的持续时间+同步延迟
    //               = 第2帧pts-第1帧pts+同步延迟
    //               = delay +同步延迟
    //               = delay + diff
    //当diff为正值，为了确保第2帧视频和音频能同步，第二帧要等待delay+diff

     //视频帧与音频帧的时钟差值:
    double diff=m_videoClock.getClock()-m_videoClock.getClock();
    //qDebug()<<"diff:"<<diff;
    //计算同步阈值
    double sync=FFMAX(AV_SYNC_THRESHOLD_MIN,FFMIN(AV_SYNC_THRESHOLD_MAX,delay));

    //不同步时间超过阈值直接放弃同步
    if(!isnan(diff)&&fabs(diff)<AV_NOSYNC_THRESHOLD) {
        if(diff<=-sync) {
            delay=FFMAX(0,diff+delay);
        }
        else if(diff>=sync&&delay>AV_SYNC_FRAMEDUP_THRESHOLD) {//帧率大10的情况
            delay=diff+delay;
        }
        else if(diff>=sync) {//帧率大于10的情况
            delay=2*delay;
        }
    }
    return delay;
}

double AVPlayer::vpDuration(MyFrame* lastFrame, MyFrame* curFrame)
{
    if(curFrame->serial==lastFrame->serial) {
        double duration=curFrame->pts-lastFrame->pts;
        if(isnan(duration)||duration>AV_NOSYNC_THRESHOLD)
            return lastFrame->duration;
        else
            return duration;

    }
    else {
        return 0.00;
    }
}
