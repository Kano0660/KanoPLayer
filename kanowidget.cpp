#include "kanowidget.h"
#include <QPushButton>
#include <QHBoxLayout>
#include <QDebug>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QApplication>
#include <QStyleOption>
#include <QPainter>
#include <QFile>
#include <QToolButton>
#include <QFileDialog>
#include <QCoreApplication>
#include <QMetaType>
#include <KanoPlayer.h>
#include <QTimer>
#include <QKeyEvent>

#define PADDING 5//边距



KanoWidget::KanoWidget(QWidget *parent)
    : QWidget(parent),m_vFmt("常见媒体格式(*.mp4 *.mov *.avi *.mkv *.wmv *.flv"
             "*.webm *.mpeg *.mpg *.3gp *.m4v *.rmvb *.vob *.ts *.mts *.m2ts *.f4v *.divx *.xvid);;"
             "所有文件(*.*);;"
             )
{

    QCoreApplication::setOrganizationName("KanoQT");
    QCoreApplication::setApplicationName("Kano Player2.0");


    //设置窗口的宽高
    Standardrect=QRect(0,0,800,600);
    setMinimumHeight(Standardrect.height());
    setMinimumWidth(Standardrect.width());
    //背景色
    setStyleSheet("background-color:rgb(30,30,30)");
    //无边框和按钮菜单
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    //开启鼠标追踪
    setMouseTracking(true);
    //为了实现鼠标的位置信息获取不受子控件的影响，启动鼠标悬浮追踪，代码如下：
    setAttribute(Qt::WA_Hover);

    InitUI();

}


void KanoWidget::InitUI(){
    //主窗体构成
    kanoTitleBar = new KanoTitleBar(this);
    kanoPlayWidget = new KanoPlayWidget(this);
    kanoPlayWidget->setObjectName(QString::fromUtf8("KanoPlayWidget"));
    kanoPlayCtrlBar = new KanoPlayCtrlBar(this);

    kanoListViewWidget =new KanoListViewWidget(this);

//    //可以将控件中的某个按钮设为默认按钮，并且设为焦点。
//    kanoOpenFile->setFocus();

    InitList();

     //QSS
    QString qssname(":/qss/kano.qss");
    QFile file(qssname);
    file.open(QFile::ReadOnly);
    QString styleSheet = tr(file.readAll());
    this->setStyleSheet(styleSheet);
    file.close();


    //计时器
    m_timer=new QTimer(this);//软件定时器，QTimer到主要功能是设置一个以毫秒为单位到定时周期，然后进行连续定时或单次定时
    m_timer->stop();
    connect(m_timer,&QTimer::timeout,this,&KanoWidget::do_timer_timeout);
    m_timer->setInterval(5000);//设置间隔
    m_timer->setSingleShot(true);
    m_timer->setTimerType(Qt::CoarseTimer);//精度

    //安装事件过滤器
    kanoTitleBar->installEventFilter(this);
    kanoPlayWidget->installEventFilter(this);
    kanoListViewWidget->installEventFilter(this);

    //主窗体布局
    Layout();

    //connect(btnList,&QPushButton::clicked,this,&KanoWidget::on_btnList);
    connect(btnPlayStatus,&QPushButton::clicked,this,&KanoWidget::on_btnPlayStatus);


    //自定义数据类型在槽中作为参数传递需先注册
    qRegisterMetaType<QSharedPointer<YUV420Frame>>("QSharedPointer<YUV420Frame>");
    connect(kanoPlayCtrlBar,&KanoPlayCtrlBar::frameChanged,kanoPlayWidget,&KanoPlayWidget::onShowYUV);

    connect(kanoPlayCtrlBar,&KanoPlayCtrlBar::sig_playNext,this,&KanoWidget::on_playNext);
    connect(kanoPlayCtrlBar,&KanoPlayCtrlBar::sig_playUpward,this,&KanoWidget::on_playUpward);
    connect(kanoPlayCtrlBar,&KanoPlayCtrlBar::sig_ShowList,this,&KanoWidget::on_ShowList);
    connect(kanoPlayCtrlBar,&KanoPlayCtrlBar::sig_OpenFile,this,&KanoWidget::m_OpenFileclicked);
    connect(kanoPlayCtrlBar,&KanoPlayCtrlBar::sig_ShowFull,this,[&](){
        if(isFullScreenBool) {
            LeaveshowFullScreen();
        }
        else {
            EntershowFullScreen();
        }
    });

    connect(this,&KanoWidget::openfile,kanoPlayCtrlBar,&KanoPlayCtrlBar::play);
    connect(kanoPlayCtrlBar,&KanoPlayCtrlBar::sig_playhide,this,&KanoWidget::slot_playhide);

    connect(kanoTitleBar,&KanoTitleBar::mouseDoubleClicked,this,[&](bool checked){
        Q_UNUSED(checked)
        if(isMaximizedBool) {
            LeaveshowMaximized();
        }
        else {
            EntershowMaximized();
        }
    });
    connect(kanoTitleBar,&KanoTitleBar::btnMaxClicked,this,[&](bool btn_isMaximizedBool){
        isMaximizedBool =btn_isMaximizedBool;
        if(isMaximizedBool) {
            LeaveshowMaximized();
        }
        else {
            EntershowMaximized();
        }
    });

    connect(kanoPlayWidget,&KanoPlayWidget::mouseDoubleClicked,this,[&](){
        if(isFullScreenBool) {
            LeaveshowFullScreen();
        }
        else {
            EntershowFullScreen();
        }
    });
    connect(kanoPlayWidget,&KanoPlayWidget::mouseClicked,kanoPlayCtrlBar,&KanoPlayCtrlBar::pauseOnBtnClickSlot);
    connect(kanoPlayWidget,&KanoPlayWidget::mouseClicked,this,&KanoWidget::onplay_mouseClicked);



    connect(kanoListViewWidget,&KanoListViewWidget::doubleClicked,this,&KanoWidget::playListViewDClickSlot);
    connect(kanoListViewWidget,&KanoListViewWidget::btnAddListClick,this,&KanoWidget::m_OpenFileclicked);

    connect(kanoTitleBar,&KanoTitleBar::btnCloseSingnal,this,[&](){
        kanoListViewWidget->setPlayListToSettings();
        this->close();

    });

    isLeftPressed = false;
    location ==CENTER;


}

void KanoWidget::Kano()
{
//当前存在问题

//1.listview会处理鼠标进入事件，导致光标转换出现异常


//尚未实现:
//1.倍速播放，重新实现音视频同步逻辑
//倍速播放原理:
//--视频倍速
//    当速率较低时:更改每帧视频的pts，例如2倍速，让每帧视频的渲染时间减半
//    当速率较高时:如果每帧渲染的时间太短并继续缩短，对导致cpu占用过高，此时应当考虑主动丢帧、或只解码关键帧等手段

//--音频倍速
//    -音频是一段连续的数字信号，如果直接更改pts，会导致丢失数据从而播放会有很多的杂音
//    -另一方面对于同一个声卡在播放音频时，不能改变他的输出速率即单位时间内获取的数据总量(样本数量)不能变
//     因此音频倍速既考虑重采样，对获取的音频帧更改采样频率
//    -如果仅更改采样频率会导致音调发生变化，一般采用第三方库解决这个问题
//2.网络流文件播放，考虑整一个边下边播



}

void KanoWidget::Layout(){
    gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(0);
    verticalLayout_2 = new QVBoxLayout();
    verticalLayout_2->setSpacing(0);
    verticalLayout_2->addWidget(kanoTitleBar);
    horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(kanoListViewWidget);

    verticalLayout = new QVBoxLayout();
    verticalLayout->setSpacing(0);
    verticalLayout->addWidget(kanoPlayWidget);
    verticalLayout->addWidget(kanoPlayCtrlBar);

    horizontalLayout->addLayout(verticalLayout);
    horizontalLayout->setSpacing(0);

    verticalLayout_2->addLayout(horizontalLayout);

    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->addLayout(verticalLayout_2, 0, 0, 1, 1);

    kanoListViewWidget->hide();
    //btnList->setGeometry(0,this->height()*0.5-10,20,40);
}


void KanoWidget::InitList(){
//    btnList = new QPushButton(this);
//    btnList->setCheckable(true);
//    btnList->setChecked(false);
//    btnList->setObjectName("btnList");
//    btnList->setFixedSize(20,40);


    btnPlayStatus =new QPushButton(this);
    btnPlayStatus->setCheckable(false);
    btnPlayStatus->setChecked(false);
    btnPlayStatus->setObjectName("btnPlayStatus");
    btnPlayStatus->setFixedSize(60,60);

    QPixmap pixmap = QPixmap(":/images/images/btnPlay.svg");
    btnPlayStatus->setIcon(pixmap);
    btnPlayStatus->setIconSize(QSize(60, 60));
    QPainter p(btnPlayStatus);
    //表示引擎应尽可能对图元的边缘进行抗锯齿。
    p.setRenderHint(QPainter::Antialiasing);

    btnPlayStatus->hide();
    kanoListViewWidget->hide();



}


void KanoWidget::UpdateUI(){


}




void KanoWidget::mousePressEvent(QMouseEvent *event)
{
    switch(event->button()){
    case Qt::RightButton:
        break;
    case Qt::LeftButton:
        isLeftPressed=true;
        if(location==CENTER){
            mouseOFFset = event->globalPos()-this->frameGeometry().topLeft();
        } 
        break;
    }



}

void KanoWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton){
        isLeftPressed = false;
        isLeftPressed_Move = false;
        isLeftPressed_Move_title=false;
        moveX=0;
        moveY=0;
    }

}

void KanoWidget::mouseMoveEvent(QMouseEvent *event)
{

    QPoint globalPos = event->globalPosition().toPoint();
    QRect rect=this->rect();
    QPoint topLeft=mapToGlobal(rect.topLeft());//0,0
    QPoint bottomRight=mapToGlobal(rect.bottomRight());//500,300

    if(isLeftPressed_Move==true){
        isLeftPressed_Move=false;
        isLeftPressed_Move_title=true;
        isLeftPressed = true;
        this->showNormal();
        this->setGeometry(moveX,moveY,Standardrect.width(),Standardrect.height());
        int i=0;

    }

    if(isFullScreenBool){//全屏状态下，显示playctrbar
        int tempy=LayoutHeight.playerctr;
        this->setCursor(QCursor(Qt::ArrowCursor));
        m_timer->stop();
        m_timer->start();
        if(globalPos.y()>=(bottomRight.y()-tempy)){
            onFullScreen_showPlayCtrBar();
        }

    }
    //最大化或全屏状态时不能拉伸
    if (isMaximized()||isFullScreen())
    {
        return;
    }

    //鼠标移动时发送位置
    if(!isLeftPressed){
        setCursorShape(globalPos);
        return;
    }
    if(isLeftPressed_Move_title==true){
        move(globalPos-mouseOFFset_T);
        event->accept();
        return;
    }


    //鼠标在CENTER位置按下
    if(location ==CENTER){
        move(globalPos-mouseOFFset);
        event->accept();
        return;
    }

    //鼠标在边框区域缩放
    QRect rMove(topLeft,bottomRight);
    switch(location){
    case TOP:
        //如果不加if判断，窗口达到最小高度后，会被鼠标"向下推走"
        if((bottomRight.y()-globalPos.y())>this->minimumHeight()){
            rMove.setY(globalPos.y());
        }

        break;
    case BOTTOM:
        rMove.setHeight(globalPos.y()-topLeft.y());
        break;
    case LEFT:
        if((bottomRight.x()-globalPos.x())>this->minimumWidth()){
            rMove.setX(globalPos.x());
        }
        break;
    case RIGHT:
        rMove.setWidth(globalPos.x()-topLeft.x());
        break;
    case TOP_LEFT:
        if(((bottomRight.x()-globalPos.x())>this->minimumWidth())&&((bottomRight.y()-globalPos.y())>this->minimumHeight())){
            rMove.setY(globalPos.y());
            rMove.setX(globalPos.x());
        }
        else if((bottomRight.x()-globalPos.x())<=this->minimumWidth()){
            //注意此时，如果鼠标仍然保持按下的状态，location的值是不会改变的，所以当窗口缩放到最小宽度时，我们仍然可以缩放窗口的宽度
            if(((bottomRight.y()-globalPos.y())>this->minimumHeight())){
                rMove.setY(globalPos.y());
            }
        }
        else if((bottomRight.y()-globalPos.y())<=this->minimumHeight()){
            if((bottomRight.x()-globalPos.x())>this->minimumWidth()){
                rMove.setX(globalPos.x());
            }
        }

        break;
    case TOP_RIGHT:
        if((bottomRight.y()-globalPos.y())>this->minimumHeight()){
            rMove.setWidth(globalPos.x()-topLeft.x());
            rMove.setY(globalPos.y());
        }
        else if((bottomRight.y()-globalPos.y())<=this->minimumHeight()){
            rMove.setWidth(globalPos.x()-topLeft.x());
        }
        break;
    case BOTTOM_LEFT:
        if((bottomRight.x()-globalPos.x())>this->minimumWidth()){
            rMove.setHeight(globalPos.y()-topLeft.y());
            rMove.setX(globalPos.x());
        }
        else if((bottomRight.x()-globalPos.x())<=this->minimumWidth()){
            rMove.setHeight(globalPos.y()-topLeft.y());
        }
        break;
    case BOTTOM_RIGHT:
        rMove.setHeight(globalPos.y()-topLeft.y());
        rMove.setWidth(globalPos.x()-topLeft.x());
        break;
    }

    this->setGeometry(rMove);//触发resizeevent

//    if(kanoListViewWidget->isHidden()){
//        btnList->move(0,this->height()*0.5-10);
//    }
//    else{
//        btnList->move(200,this->height()*0.5-10);
//    }


}


void KanoWidget::m_OpenFileclicked()
{
    playFileUrlList = QFileDialog::getOpenFileNames(this,"选择媒体文件(可多选)",QCoreApplication::applicationDirPath(),m_vFmt);
    if(playFileUrlList.isEmpty())
        return;
    kanoListViewWidget->OpenFiles(playFileUrlList);
    playFileUrl = playFileUrlList.at(0);

    currentPlayFileUrl = playFileUrl;
    if(!playFileUrl.isEmpty()){
        emit openfile(playFileUrl);
    }
}

void KanoWidget::slot_playhide(bool checked)
{
    playstatus =checked;
}



void KanoWidget::setCursorShape(const QPoint &globalPos)
{
    QRect rect=this->rect();
    QPoint topLeft=mapToGlobal(rect.topLeft());
    QPoint bottomRight=mapToGlobal(rect.bottomRight());

    int x = globalPos.x();
    int y = globalPos.y();


    int topXOFFset = topLeft.x()+PADDING;
    int topYOFFset = topLeft.y()+PADDING;
    int bomXOFFset = bottomRight.x()-PADDING;
    int bomYOFFset = bottomRight.y()-PADDING;

    if(x>=topLeft.x()&&x<=topXOFFset && y>=topLeft.y()&&y<=topYOFFset){
        //左上
        location =TOP_LEFT;
        this->setCursor(QCursor(Qt::SizeFDiagCursor));//鼠标形状
    }
    else if(x>=topLeft.x()&&x<=topXOFFset&&y>=topYOFFset&&y<=bomYOFFset){
        //左边
        location =LEFT;
        //CursorShahp(globalPos);
        this->setCursor(QCursor(Qt::SizeHorCursor));

    }
    else if(x>=topLeft.x()&&x<=topXOFFset && y>=bomYOFFset&&y<=bottomRight.y()){
        //左下
        location =BOTTOM_LEFT;
        this->setCursor(QCursor(Qt::SizeBDiagCursor));//鼠标形状
    }
    else if(y>=topLeft.y()&&y<=topYOFFset&&x>=topXOFFset&&x<=bomXOFFset){
        //上面
        location =TOP;
        this->setCursor(QCursor(Qt::SizeVerCursor));//鼠标形状
    }
    else if(y<=bottomRight.y()&&y>=bomYOFFset&&x>=topXOFFset&&x<=bomXOFFset){
        //下面
        location =BOTTOM;
        this->setCursor(QCursor(Qt::SizeVerCursor));//鼠标形状
    }
    else if(x>=bomXOFFset&&x<=bottomRight.x()&&y>=topLeft.y()&&y<=topYOFFset){
        //右上
        location =TOP_RIGHT;
        this->setCursor(QCursor(Qt::SizeBDiagCursor));//鼠标形状
    }
    else if(x>=bomXOFFset&&x<=bottomRight.x()&&y>=topYOFFset&&y<=bomYOFFset){
        //右边
        location =RIGHT;
        this->setCursor(QCursor(Qt::SizeHorCursor));
    }
    else if(x>=bomXOFFset&&x<=bottomRight.x()&&y>=bomYOFFset&&y<=bottomRight.y()){
        //右下
        location =BOTTOM_RIGHT;
           //CursorShahp(globalPos);
        this->setCursor(QCursor(Qt::SizeFDiagCursor));//鼠标形状
    }
    else{
        location =CENTER;
        this->setCursor(QCursor(Qt::ArrowCursor));
    }




}

void KanoWidget::CursorShahp(const QPoint &globalPos){


    QRect rect=this->rect();
    QPoint topLeft=mapToGlobal(rect.topLeft());
    QPoint bottomRight=mapToGlobal(rect.bottomRight());

    int x = globalPos.x();
    int y = globalPos.y();

    if(y>=(topLeft.y()+LayoutHeight.title)&&y<=(bottomRight.y()-LayoutHeight.playerctr)){
        this->setCursor(QCursor(Qt::ArrowCursor));
    }
    else{
        this->setCursor(QCursor(Qt::SizeHorCursor));//鼠标形状
    }
}

void KanoWidget::resizeEvent(QResizeEvent *event)
{
    UpdateUI();
}



void KanoWidget::on_btnList(bool checked)
{
    if(kanoListViewWidget->isHidden()){
        kanoListViewWidget->show();
        btnList->setGeometry(200,this->height()*0.5-10,20,40);
        update();
    }
    else{
        btnList->setGeometry(0,this->height()*0.5-10,20,40);
        kanoListViewWidget->hide();
        update();
    }
}

void KanoWidget::on_btnPlayStatus(bool checked)
{


}

void KanoWidget::on_playNext()
{
    int index=kanoListViewWidget->getUrlSerial(currentPlayFileUrl);
    if(index == -1)
        return;
    if(index==kanoListViewWidget->getPlayListNumber()-1){
        currentPlayFileUrl=kanoListViewWidget->getUrl(0);
        kanoListViewWidget->setCurrentIndex(0);
    }
    else{
        currentPlayFileUrl=kanoListViewWidget->getUrl(index+1);
        kanoListViewWidget->setCurrentIndex(index+1);
    }

    emit openfile(currentPlayFileUrl);
}

void KanoWidget::on_playUpward()
{
    int index=kanoListViewWidget->getUrlSerial(currentPlayFileUrl);
    if(index == -1)
        return;
    if(index==0){
        currentPlayFileUrl=kanoListViewWidget->getUrl(kanoListViewWidget->getPlayListNumber()-1);
        kanoListViewWidget->setCurrentIndex(kanoListViewWidget->getPlayListNumber()-1);
    }
    else{
        currentPlayFileUrl=kanoListViewWidget->getUrl(index-1);
        kanoListViewWidget->setCurrentIndex(index-1);
    }

    emit openfile(currentPlayFileUrl);
}

void KanoWidget::on_ShowList()
{
    if(kanoListViewWidget->isHidden()){
        kanoListViewWidget->show();
        //btnList->move(200,this->height()*0.5-10);
    }
    else{
        kanoListViewWidget->hide();
        //btnList->move(0,this->height()*0.5-10);
    }

}

void KanoWidget::addWidget(QWidget)
{

}






void KanoWidget::do_timer_timeout(){
    kanoPlayCtrlBar->hide();
    this->setCursor(QCursor(Qt::BlankCursor));
}

void KanoWidget::playListViewDClickSlot(const QModelIndex &index)
{
    QString listindex=index.data(Qt::DisplayRole).toString();
    if(currentPlayFileUrl==listindex)//当前播放文件和双击为同一个文件
        return;

    currentPlayFileUrl = listindex;
    emit openfile(listindex);

}

void KanoWidget::onplay_mouseClicked()
{


}

void KanoWidget::LeaveshowFullScreen()
{
    kanoTitleBar->show();
    kanoPlayCtrlBar->show();
    //btnList->show();

    m_timer->stop();
    this->setCursor(QCursor(Qt::ArrowCursor));


    isFullScreenBool = false;
    if(isMaximizedBool==true){//最大化的情况下进入全屏，会导致showNormal不能恢复至最大化状态
        this->showMaximized();
    }
    else{
        this->showNormal();
    }
    this->showNormal();

    //btnList->move(0,this->height()*0.5-10);

    qDebug()<<"ismax = "<<isMaximized();
}

void KanoWidget::onFullScreen_showPlayCtrBar()
{
    if(kanoPlayCtrlBar->isHidden()){
        kanoPlayCtrlBar->show();
    }

}
void KanoWidget::EntershowFullScreen()
{
    this->showFullScreen();
    m_timer->start();
    isFullScreenBool = true;
    kanoTitleBar->hide();
   // btnList->hide();
    kanoPlayCtrlBar->hide();
    kanoListViewWidget->hide();
}



void KanoWidget::EntershowMaximized()
{
    this->showMaximized();
    isMaximizedBool =true;
    int temp= this->height();
    //btnList->move(0,this->height()*0.5-10);

}



void KanoWidget::LeaveshowMaximized()
{
   this->showNormal();
   isMaximizedBool =false;
   int temp= this->height();
   //btnList->move(0,this->height()*0.5-10);
}




void KanoWidget::keyPressEvent(QKeyEvent *event)
{

   if (event->key() == Qt::Key_Escape)
    {
        if (this->isFullScreen())
        {
            LeaveshowFullScreen();
        }
    }
   else if(event->key() == Qt::Key_Space){
        if (playstatus){
            if(!playstatus_keyspace){
                playstatus_keyspace = true;
                btnPlayStatus->show();
                kanoPlayCtrlBar->btnPlayClick();
                btnPlayStatus->move(this->width()*0.5,this->height()*0.5);
            }

            else{
                playstatus_keyspace = false;
                kanoPlayCtrlBar->btnPlayClick();
                btnPlayStatus->hide();
            }
        }
   }

}


bool KanoWidget::eventFilter(QObject *watched, QEvent *event)
{

    if((watched==kanoTitleBar)&&(event->type()==QEvent::MouseButtonPress))
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if ((mouseEvent->button() == Qt::LeftButton)&&(this->isMaximized())){
            isLeftPressed_Move=true;
            QScreen* screen = QGuiApplication::primaryScreen();//获取主屏幕
            //geometry()返回的是屏幕的大小，即屏幕分辨率大小，包括屏幕下方的工具栏（1090*1080）
            QRect rect1 = screen->geometry();
            QPoint globalPos =  mouseEvent->globalPosition().toPoint();
            moveX=globalPos.x()-0.5*Standardrect.width();
            int moveX_R=rect1.right()-globalPos.x()-0.5*Standardrect.width();
            if(moveX>=0){
                if(moveX_R>=0){
                    moveX = moveX;
                }
                else{
                    moveX = rect1.right()-Standardrect.width();
                }
            }
            else {
                moveX = 0;
            }
            moveY=globalPos.y()-5;
            moveY = moveY>0 ? moveY : 0;
            mouseOFFset_T.setX(mouseEvent->globalPos().x()-moveX);
            mouseOFFset_T.setY(mouseEvent->globalPos().y()-moveY);
        }
        return false;//ruturn false，继续传递
    }
    else if(watched==kanoListViewWidget){
        if(event->type()==QEvent::Enter){
            //btnList->show();
        }
        return false;
    }

    return false;//ruturn false，继续传递

}

KanoWidget::~KanoWidget()
{
}

