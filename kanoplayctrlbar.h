#ifndef KANOPLAYCTRLBAR_H
#define KANOPLAYCTRLBAR_H

#include <QWidget>
#include "KanoPalyer/av_player.h"
#include "KanoWidget/KanoSlider.h"

class QMenu;
class QPushButton;
class QLabel;
class QSlider;
class QGridLayout;
class QVBoxLayout;
class QHBoxLayout;
class QSpacerItem;

struct PCBLayoutStruct{//PlayCtrlBarLayout
    int Mwidth =100;
    int Swidth = 60;
    int Sheight = 5;
    int btnLarge = 35;
    int btnMediu = 28;
    int btnSmall = 15;
};



class KanoPlayCtrlBar : public QWidget
{
    Q_OBJECT
public:
    explicit KanoPlayCtrlBar(QWidget *parent = nullptr);

public:
    void btnPlayClick();

signals:
    void sig_playRate(float speed);
    void frameChanged(QSharedPointer<YUV420Frame> frame);
    void sig_playhide(bool checked);
    void sig_OpenFile();
    void sig_ShowFull();
    void sig_ShowList();
    void sig_playNext();
    void sig_playUpward();
private:
    void Init();
    void InitUI();
    void InitConnect();
    void UpdateUI();
    void InitGeometry();
    //播放时隐藏ctrlbar控件
    void playHide();
    void Layout();

private:
    bool playfile = false;//是否有文件在播放
    unsigned int m_duration;//视频时长
    unsigned int m_pts;//播放进度
    unsigned int m_volume = 50;


    PCBLayoutStruct PCBLayout;
    //显示列表按钮
    QPushButton *btnShowList;
    //播放进度条和进度显示标签
    QLabel *durationLabel;
    KanoPlaySlider *durationSlider;
    bool durationSliderPressed = false;
    int m_seekTarget = 0;
    //停止放映按钮
    QPushButton *btnStop;
    //上一部按钮
    QPushButton *btnUpward;
    //下一步按钮
    QPushButton *btnNext;

    //声音按钮和声音进度条按钮
    QPushButton *btnSound;
    QSlider *volumeSlider;

    //播放和暂停
    AVPlayer  *m_player;
    QPushButton *bBtnPlay;
    QString palyFielUrl;


    //播放速度
    QPushButton *bBtnPlayspeed;
    QMenu* pMenu;


     //打开文件按钮
    QPushButton *btnOpenFile;
     //全屏按钮
    QPushButton *btnShowFullScreen;



    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout_6;
    QVBoxLayout *verticalLayout_5;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout;

    QHBoxLayout *horizontalLayout_5;
    QSpacerItem *horizontalSpacer_12;
    QSpacerItem *horizontalSpacer_13;
    QSpacerItem *horizontalSpacer_14;
    QSpacerItem *horizontalSpacer_15;

    QSpacerItem *verticalSpacer_4;
    QVBoxLayout *verticalLayout_4;
    QVBoxLayout *verticalLayout_3;
    QSpacerItem *verticalSpacer_3;
    QHBoxLayout *horizontalLayout_9;

    QSpacerItem *horizontalSpacer_5;
    QSpacerItem *horizontalSpacer_7;
    QSpacerItem *horizontalSpacer_8;
    QSpacerItem *horizontalSpacer_6;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_4;
    QHBoxLayout *horizontalLayout_6;
    QVBoxLayout *verticalLayout_9;
    QSpacerItem *verticalSpacer_9;

    QSpacerItem *verticalSpacer_10;
    QVBoxLayout *verticalLayout_7;
    QSpacerItem *verticalSpacer_5;

    QSpacerItem *verticalSpacer_6;

    QVBoxLayout *verticalLayout_8;
    QSpacerItem *verticalSpacer_7;

    QSpacerItem *verticalSpacer_8;
    QHBoxLayout *horizontalLayout_3;

    QVBoxLayout *verticalLayout;
    QSpacerItem *verticalSpacer;

    QSpacerItem *verticalSpacer_2;
    QSpacerItem *horizontalSpacer_9;

    QSpacerItem *horizontalSpacer_10;
    QHBoxLayout *horizontalLayout_4;



public slots:
    void play(const QString url);
    void pauseOnBtnClickSlot();
private slots:

    void seekForwardSlot();//进度向前
    void seekBackSlot();//进度向后
    void on_btnPlay_clicked(bool checked=false);
    void on_PlayChanged(bool checked=false);
    void on_btnShowList_clicked(bool checked=false);
    void frameChanged_slots(QSharedPointer<YUV420Frame> frame);
    void on_btnSound_clicked(bool checked = false);
    void on_btnOpenFile_clicked(bool checked = false);
    void on_btnShowFullScreen_clicked(bool checked =false);
    void setVolume(int volume);//声音大小
    void durationChangedSlot(unsigned int duration);//视频时长
    void ptsChangedSlot(unsigned int pts);//当前播放进度
    void terminateSlot();
    void ptsSliderPressedSlot();
    void ptsSliderMovedSlot();
    void ptsSliderReleaseSlot();

    void on_playrate_changed(float speed);

    // QObject interface
public:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
};



#endif // KANOPLAYCTRLBAR_H
