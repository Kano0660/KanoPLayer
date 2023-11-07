#include "kanoplayctrlbar.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSvgRenderer>
#include <QPainter>
#include <QMouseEvent>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QSlider>
#include <QGridLayout>
#include <QSpacerItem>
#include "KanoPlayer.h"
#include <QSvgRenderer>

KanoPlayCtrlBar::KanoPlayCtrlBar(QWidget *parent)
    : QWidget{parent}
{
    Init();
    InitUI();
    InitConnect();
}

void KanoPlayCtrlBar::btnPlayClick()
{
    bBtnPlay->click();
}

void KanoPlayCtrlBar::Init(){
    this->setAttribute(Qt::WA_DeleteOnClose);
    //禁止父窗口影响子窗口样式
    this->setAttribute(Qt::WA_StyledBackground);
    //背景色
    this->setStyleSheet("background-color:rgb(64,64,64);");
    setFixedHeight(80);
    //开启鼠标追踪
    setMouseTracking(true);
    //为了实现鼠标的位置信息获取不受子控件的影响，启动鼠标悬浮追踪，代码如下：
    setAttribute(Qt::WA_Hover);

}

void KanoPlayCtrlBar::InitUI(){
    //KanoPlayer
    m_player =new AVPlayer;
    m_player->setVolume(m_volume);


    //显示列表按钮
    btnShowList =new QPushButton(this);
    btnShowList->setObjectName("btnShowList");
    btnShowList->setIcon(QIcon(":/images/images/btnShowList.svg"));

    //播放进度条和进度显示标签
    durationSlider = new KanoPlaySlider(this);
    durationSlider->setObjectName("durationSlider");

    durationLabel =new QLabel(this);
    durationLabel->setObjectName("durationLabel");
    durationLabel->setText("00:00 / 00:00");
    durationLabel->setFixedSize(140,20);


    //停止放映按钮
    btnStop =new QPushButton(this);
    btnStop->setObjectName("btnStop");
    btnStop->setIcon(QIcon(":/images/images/btnStop.svg"));
    //上一部按钮
    btnUpward =new QPushButton(this);
    btnUpward->setObjectName("btnUpward");
    btnUpward->setIcon(QIcon(":/images/images/btnUpward.svg"));
    //下一部按钮
    btnNext =new QPushButton(this);
    btnNext->setObjectName("btnNext");
    btnNext->setIcon(QIcon(":/images/images/btnNext.svg"));


    //播放和暂停按钮
    bBtnPlay =new QPushButton(this);
    bBtnPlay->setObjectName("bBtnPlay");
    QPixmap pixmap = QPixmap(":/images/images/btnPlay.svg");
    bBtnPlay->setIcon(pixmap);
    bBtnPlay->setIconSize(QSize(30, 30));
    QPainter p(bBtnPlay);
    //表示引擎应尽可能对图元的边缘进行抗锯齿。
    p.setRenderHint(QPainter::Antialiasing);
    bBtnPlay->setCheckable(true);
    bBtnPlay->setChecked(false);//默认未被按下





    //声音按钮和声音进度条按钮
    btnSound = new QPushButton(this);
    btnSound->setObjectName("btnSound");
    btnSound->setCheckable(true);
    btnSound->setChecked(false);
    btnSound->setIcon(QIcon(":/images/images/btnSound_volumn.svg"));

    volumeSlider = new QSlider(this);
    volumeSlider->setObjectName("volumeSlider");
    volumeSlider->setValue(m_volume);

    //倍速按钮和菜单
    bBtnPlayspeed = new QPushButton(this);
    bBtnPlayspeed ->setObjectName("bBtnPlayspeed");
    bBtnPlayspeed->setText(tr("正常倍速"));
    bBtnPlayspeed->setStyleSheet("QPushButton{ \
        background-color:rgb(70,70,70);;color:white;border:none; \
        text-align:right;font-size: 12px; \
        border-radius:6px;} \
        QPushButton:hover{background-color:rgb(99, 99, 99);border:none;} \
        QPushButton:pressed{border:none;} \
        QPushButton::menu-indicator{image:none}");

    QMenu* pMenu = new QMenu(this);

    QAction* pAc1 = new QAction(tr("0.5 倍速"), this);
    QAction* pAc2 = new QAction(tr("0.75 倍速"), this);
    QAction* pAc3 = new QAction(tr("1.0 倍速"), this);
    QAction* pAc4 = new QAction(tr("1.25 倍速"), this);
    QAction* pAc5 = new QAction(tr("1.5 倍速"), this);
    QAction* pAc6 = new QAction(tr("1.75 倍速"), this);
    QAction* pAc7 = new QAction(tr("2.0 倍速"), this);
    pMenu->addAction(pAc1);
    pMenu->addAction(pAc2);
    pMenu->addAction(pAc3);
    pMenu->addAction(pAc4);
    pMenu->addAction(pAc5);
    pMenu->addAction(pAc6);
    pMenu->addAction(pAc7);

     bBtnPlayspeed->setMenu(pMenu);

    pMenu->installEventFilter(this);  //设置事件过滤，调整弹出菜单的位置

    connect(pAc1, &QAction::triggered, [=] {
         bBtnPlayspeed->setText("0.5 倍速");
        emit sig_playRate(0.5);
    });

    connect(pAc2, &QAction::triggered, [=] {
          bBtnPlayspeed->setText("0.75 倍速");
        emit sig_playRate(0.75);
    });

    connect(pAc3, &QAction::triggered, [=] {
          bBtnPlayspeed->setText("正常倍速");
        emit sig_playRate(1.0);
    });

    connect(pAc4, &QAction::triggered, [=] {
          bBtnPlayspeed->setText("1.25 倍速");
        emit sig_playRate(1.25);
    });

    connect(pAc5, &QAction::triggered, [=] {
          bBtnPlayspeed->setText("1.5 倍速");
        emit sig_playRate(1.5);
    });

    connect(pAc6, &QAction::triggered, [=] {
          bBtnPlayspeed->setText("1.75 倍速");
        emit sig_playRate(1.75);
    });

    connect(pAc7, &QAction::triggered, [=] {
          bBtnPlayspeed->setText("2.0 倍速");
        emit sig_playRate(2.0);
    });

    std::string menuItemQss = R"(
        QMenu
        {
            background-color:rgb(73, 73, 73);
        }

        QMenu::item
        {
             font:16px;
             color:white;
             background-color:rgb(73, 73, 73);
             margin:8px 8px;
        }

        /*选择项设置*/
        QMenu::item:selected
        {
             background-color:rgb(54, 54, 54);
        }
    )";

    pMenu->setStyleSheet(QString::fromStdString(menuItemQss));
    pMenu->setWindowOpacity(0.7);


    //打开文件按钮
    btnOpenFile = new QPushButton(this);
    btnOpenFile->setObjectName("btnOpenFile");
    btnOpenFile->setIcon(QIcon(":/images/images/btnOpenFile.svg"));

    //全屏按钮
    btnShowFullScreen = new QPushButton(this);
    btnShowFullScreen ->setObjectName("btnShowFullScreen");
    btnShowFullScreen->setIcon(QIcon(":/images/images/btnShowFullScreen_M.svg"));




    durationSlider->setEnabled(false);

    UpdateUI();
}

void KanoPlayCtrlBar::InitConnect()
{
    //自定义数据类型在槽中作为参数传递需先注册
    qRegisterMetaType<QSharedPointer<YUV420Frame>>("QSharedPointer<YUV420Frame>");
    connect(m_player,&AVPlayer::frameChanged,this,&KanoPlayCtrlBar::frameChanged_slots,Qt::QueuedConnection);//解码成功后发送帧数据给openglwidget
    connect(bBtnPlay,&QPushButton::clicked,this,&KanoPlayCtrlBar::on_btnPlay_clicked);
    //    connect(btnForward,&QPushButton::clicked,this,&KanoPlayCtrlBar::seekForwardSlot);
    //    connect(btnBack,&QPushButton::clicked,this,&KanoPlayCtrlBar::seekBackSlot);//进度向后
    connect(btnNext,&QPushButton::clicked,this,&KanoPlayCtrlBar::on_PlayChanged);
    connect(btnUpward,&QPushButton::clicked,this,&KanoPlayCtrlBar::on_PlayChanged);
    connect(btnShowList,&QPushButton::clicked,this,&KanoPlayCtrlBar::on_btnShowList_clicked);

    connect(btnSound,&QPushButton::clicked,this,&KanoPlayCtrlBar::on_btnSound_clicked);
    connect(volumeSlider,&QSlider::valueChanged,this,&KanoPlayCtrlBar::setVolume);
    connect(m_player,&AVPlayer::AVDurationChanged,this,&KanoPlayCtrlBar::durationChangedSlot);
    connect(m_player,&AVPlayer::AVPtsChanged,this,&KanoPlayCtrlBar::ptsChangedSlot);
    connect(m_player,&AVPlayer::AVTerminate,this,&KanoPlayCtrlBar::terminateSlot,Qt::QueuedConnection);


    connect(durationSlider,&KanoPlaySlider::sliderPressed,this,&KanoPlayCtrlBar::ptsSliderPressedSlot);
    connect(durationSlider,&KanoPlaySlider::sliderMoved,this,&KanoPlayCtrlBar::ptsSliderMovedSlot);
    connect(durationSlider,&KanoPlaySlider::sliderReleased,this,&KanoPlayCtrlBar::ptsSliderReleaseSlot);

    connect(btnOpenFile,&QPushButton::clicked,this,&KanoPlayCtrlBar::on_btnOpenFile_clicked);
    connect(btnShowFullScreen,&QPushButton::clicked,this,&KanoPlayCtrlBar::on_btnShowFullScreen_clicked);


    connect(this,&KanoPlayCtrlBar::sig_playRate,this,&KanoPlayCtrlBar::on_playrate_changed);

}


void KanoPlayCtrlBar::UpdateUI(){

    Layout();
}
void KanoPlayCtrlBar::Layout(){


    gridLayout = new QGridLayout(this);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    verticalLayout_6 = new QVBoxLayout();
    verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
    verticalLayout_5 = new QVBoxLayout();
    verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
    verticalLayout_2 = new QVBoxLayout();
    verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));

    durationSlider->setMinimumSize(QSize(50, 10));
    durationSlider->setOrientation(Qt::Horizontal);
    horizontalLayout->addWidget(durationSlider);
    horizontalLayout->setStretch(0, 2);

    verticalLayout_2->addLayout(horizontalLayout);
    horizontalLayout_5 = new QHBoxLayout();
    horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
    horizontalSpacer_12 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_5->addItem(horizontalSpacer_12);

    horizontalSpacer_13 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_5->addItem(horizontalSpacer_13);

    horizontalSpacer_14 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_5->addItem(horizontalSpacer_14);

    horizontalSpacer_15 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_5->addItem(horizontalSpacer_15);


    durationLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    horizontalLayout_5->addWidget(durationLabel);
    horizontalLayout_5->setStretch(0, 2);
    horizontalLayout_5->setStretch(1, 2);
    horizontalLayout_5->setStretch(3, 2);
    horizontalLayout_5->setStretch(4, 1);
    verticalLayout_2->addLayout(horizontalLayout_5);
    verticalLayout_5->addLayout(verticalLayout_2);
    verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout_5->addItem(verticalSpacer_4);

    verticalLayout_5->setStretch(0, 1);

    verticalLayout_6->addLayout(verticalLayout_5);

    verticalLayout_4 = new QVBoxLayout();
    verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
    verticalLayout_3 = new QVBoxLayout();
    verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
    verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);


    verticalLayout_3->addItem(verticalSpacer_3);


    verticalLayout_4->addLayout(verticalLayout_3);

    horizontalLayout_9 = new QHBoxLayout();
    horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
    btnShowList->setFixedSize(QSize(25,25));
    horizontalLayout_9->addWidget(btnShowList);

    horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_9->addItem(horizontalSpacer_5);

    horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_9->addItem(horizontalSpacer_7);

    horizontalSpacer_8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_9->addItem(horizontalSpacer_8);

    horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_9->addItem(horizontalSpacer_6);

    horizontalLayout_2 = new QHBoxLayout();
    horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));

    horizontalLayout_9->addLayout(horizontalLayout_2);

    horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_9->addItem(horizontalSpacer_4);

    horizontalLayout_6 = new QHBoxLayout();
    horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
    verticalLayout_9 = new QVBoxLayout();
    verticalLayout_9->setObjectName(QString::fromUtf8("verticalLayout_9"));
    verticalSpacer_9 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout_9->addItem(verticalSpacer_9);

    btnStop->setMinimumSize(QSize(25, 25));
    btnStop->setMaximumSize(QSize(25, 25));

    verticalLayout_9->addWidget(btnStop);

    verticalSpacer_10 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout_9->addItem(verticalSpacer_10);


    horizontalLayout_6->addLayout(verticalLayout_9);

    verticalLayout_7 = new QVBoxLayout();
    verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
    verticalSpacer_5 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout_7->addItem(verticalSpacer_5);

    btnUpward->setMinimumSize(QSize(25, 25));
    btnUpward->setMaximumSize(QSize(25, 25));

    verticalLayout_7->addWidget(btnUpward);

    verticalSpacer_6 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout_7->addItem(verticalSpacer_6);


    horizontalLayout_6->addLayout(verticalLayout_7);

    bBtnPlay->setFixedSize(QSize(30, 30));


    horizontalLayout_6->addWidget(bBtnPlay);

    verticalLayout_8 = new QVBoxLayout();
    verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
    verticalSpacer_7 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout_8->addItem(verticalSpacer_7);

    btnNext->setMinimumSize(QSize(25, 25));
    btnNext->setMaximumSize(QSize(25, 25));

    verticalLayout_8->addWidget(btnNext);

    verticalSpacer_8 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout_8->addItem(verticalSpacer_8);


    horizontalLayout_6->addLayout(verticalLayout_8);


    horizontalLayout_9->addLayout(horizontalLayout_6);

    horizontalLayout_3 = new QHBoxLayout();
    horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));

    btnSound->setMinimumSize(QSize(20, 20));
    btnSound->setMaximumSize(QSize(20, 20));

    horizontalLayout_3->addWidget(btnSound);

    verticalLayout = new QVBoxLayout();
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout->addItem(verticalSpacer);


    volumeSlider->setMinimumSize(QSize(70, 10));
    volumeSlider->setMaximumSize(QSize(70, 10));
    volumeSlider->setOrientation(Qt::Horizontal);

    verticalLayout->addWidget(volumeSlider);

    verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout->addItem(verticalSpacer_2);

    verticalLayout->setStretch(1, 2);

    horizontalLayout_3->addLayout(verticalLayout);


    horizontalLayout_9->addLayout(horizontalLayout_3);

    horizontalSpacer_9 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_9->addItem(horizontalSpacer_9);


    bBtnPlayspeed->setMinimumSize(QSize(70, 30));
    bBtnPlayspeed->setMaximumSize(QSize(70, 30));
    horizontalLayout_9->addWidget(bBtnPlayspeed);

    horizontalSpacer_10 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_9->addItem(horizontalSpacer_10);

    horizontalLayout_4 = new QHBoxLayout();
    horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));

    btnOpenFile->setMinimumSize(QSize(20, 20));
    btnOpenFile->setMaximumSize(QSize(20, 20));

    horizontalLayout_4->addWidget(btnOpenFile);


    btnShowFullScreen->setMinimumSize(QSize(20, 20));
    btnShowFullScreen->setMaximumSize(QSize(20, 20));

    horizontalLayout_4->addWidget(btnShowFullScreen);


    horizontalLayout_9->addLayout(horizontalLayout_4);

    horizontalLayout_9->setStretch(6, 1);
    horizontalLayout_9->setStretch(9, 1);
    verticalLayout_4->addLayout(horizontalLayout_9);


    verticalLayout_6->addLayout(verticalLayout_4);


    gridLayout->addLayout(verticalLayout_6, 0, 0, 1, 1);


}


void KanoPlayCtrlBar::play(const QString url)
{
    palyFielUrl = url;
    bBtnPlay->setChecked(false);
    bBtnPlay->click();
    playfile =false;

}
#include <QTimer>

void KanoPlayCtrlBar::on_btnPlay_clicked(bool checked)
{
    if(checked){
        if(playfile == true){
            //当前有文件播放，进行暂停和播放状态切换
            m_player->pause(false);
            QIcon icon(":/images/images/btnPause.svg");
            bBtnPlay->setIcon(icon);
        }

        if(palyFielUrl.count()&&playfile == false) {
            if(m_player->play(palyFielUrl)) {

                playfile=true;
                durationSlider->setEnabled(true);

                playHide();
            }
            QIcon icon(":/images/images/btnPause.svg");
            bBtnPlay->setIcon(icon);
        }

    }
    else{
        QIcon icon(":/images/images/btnPlay.svg");
        bBtnPlay->setIcon(icon);
        m_player->pause(true);

    }

}

void KanoPlayCtrlBar::on_PlayChanged(bool checked)
{
    QPushButton *btn=qobject_cast<QPushButton *>(sender());
    if(btn==btnNext){
        emit sig_playNext();
    }
    else if(btn==btnUpward) {
        emit sig_playUpward();
    }

}

void KanoPlayCtrlBar::on_btnShowList_clicked(bool checked)
{
    Q_UNUSED(checked)
    emit sig_ShowList();
}

void KanoPlayCtrlBar::playHide(){
    emit sig_playhide(true);//通知主界面隐藏
}
void KanoPlayCtrlBar::frameChanged_slots(QSharedPointer<YUV420Frame> frame)
{
    emit frameChanged(frame);

}

void KanoPlayCtrlBar::on_btnSound_clicked(bool checked)
{
    if(!checked){
        btnSound->setIcon(QIcon(":/images/images/btnSound_volumn.svg"));
        m_player->setVolume(m_volume);
    }
    else{
        btnSound->setIcon(QIcon(":/images/images/btnSound_mute.svg"));
        m_volume =volumeSlider->value();
        m_player->setVolume(0);
    }

}
void KanoPlayCtrlBar::on_btnOpenFile_clicked(bool checked){
    Q_UNUSED(checked)
    emit sig_OpenFile();
}

void KanoPlayCtrlBar::on_btnShowFullScreen_clicked(bool checked)
{
    Q_UNUSED(checked)
    emit sig_ShowFull();

}

void KanoPlayCtrlBar::seekForwardSlot()
{
    m_player->seekBy(6);
    if(m_player->playState()==AVPlayer::AV_PAUSED)
        m_player->pause(false);
}

void KanoPlayCtrlBar::seekBackSlot()
{
    m_player->seekBy(-6);
    if(m_player->playState()==AVPlayer::AV_PAUSED)
        m_player->pause(false);
}


void KanoPlayCtrlBar::setVolume(int volume)
{
    m_player->setVolume(volume);
}
void KanoPlayCtrlBar::durationChangedSlot(unsigned int duration)
{
    durationLabel->setText(QString("%3:%4 / %1:%2").arg(duration/60,2,10,QLatin1Char('0'))
                               .arg(duration%60,2,10,QLatin1Char('0'))
                               .arg(0,2,10,QLatin1Char('0'))
                               .arg(0,2,10,QLatin1Char('0')));
    m_duration=duration;
}

void KanoPlayCtrlBar::ptsChangedSlot(unsigned int pts)
{
    if(durationSliderPressed)
        return;
    durationSlider->setPtsPercent((double)pts/m_duration);
    durationLabel->setText(QString("%1:%2 / %3:%4").arg(pts/60,2,10,QLatin1Char('0'))
                               .arg(pts%60,2,10,QLatin1Char('0'))
                               .arg(m_duration/60,2,10,QLatin1Char('0'))
                               .arg(m_duration%60,2,10,QLatin1Char('0')));
    m_pts =pts;
    //qDebug()<<"update pts"<<pts<<endl;
}

void KanoPlayCtrlBar::terminateSlot()
{
    durationLabel->setText(QString("00:00 / 00:00"));
    durationSlider->setEnabled(false);
    m_player->clearPlayer();
}

void KanoPlayCtrlBar::ptsSliderPressedSlot()
{
    durationSliderPressed=true;
    m_seekTarget=(int)(durationSlider->ptsPercent()*m_duration);
}

void KanoPlayCtrlBar::ptsSliderMovedSlot()
{
    //qDebug()<<"ptsSlider value:"<<endl;
    m_seekTarget=(int)(durationSlider->cursorXPercent()*m_duration);
    const QString& ptsStr=QString("%1:%2 / %3:%4").arg(m_seekTarget/60,2,10,QLatin1Char('0'))
                                                     .arg(m_seekTarget%60,2,10,QLatin1Char('0'))
                                                     .arg(m_duration/60,2,10,QLatin1Char('0'))
                                                     .arg(m_duration%60,2,10,QLatin1Char('0'));
    if(durationSliderPressed)
         durationLabel->setText(ptsStr);
    else
        durationSlider->setToolTip(ptsStr);
}

void KanoPlayCtrlBar::ptsSliderReleaseSlot()
{
    m_player->seekTo(m_seekTarget);
    durationSliderPressed=false;
}

void KanoPlayCtrlBar::on_playrate_changed(float speed)
{
    m_player->on_playrate_changed(speed);
    if(m_player->playState()==AVPlayer::AV_PAUSED||m_player->playState()==AVPlayer::AV_PLAYING){
        m_seekTarget=(int)(durationSlider->ptsPercent()*m_duration);
        m_player->play(palyFielUrl);
        m_player->seekTo(m_seekTarget);
    }



}


void KanoPlayCtrlBar::pauseOnBtnClickSlot()
{
    bBtnPlay->click();

}



bool KanoPlayCtrlBar::eventFilter(QObject *watched, QEvent *event)
{

    if ( bBtnPlayspeed)
    {
        if (event->type() == QEvent::Show && watched ==   bBtnPlayspeed->menu())
        {
            int menuWidth =   bBtnPlayspeed->menu()->width();
            int menuHeight =   bBtnPlayspeed->menu()->height();
            int buttonWidth =   bBtnPlayspeed->width();
            int buttonHeight =   bBtnPlayspeed->height();
            int menuXPos =   bBtnPlayspeed->menu()->pos().x();
            int menuYPos =   bBtnPlayspeed->mapToGlobal(QPoint(0, 0)).y() - menuHeight - 3;
              bBtnPlayspeed->menu()->move(menuXPos, menuYPos);
            return true;
        }
    }

    return false;

}
