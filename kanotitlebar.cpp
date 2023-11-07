#include "kanotitlebar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSvgRenderer>
#include <QPainter>
#include <QMouseEvent>
//#include <Windows.h>
//#include <qt_windows.h>
//#pragma comment(lib, "user32.lib")
KanoTitleBar::KanoTitleBar(QWidget *parent)
    : QWidget{parent}
{
    Init();
    InitUI();
    RestoreUI();

}

void KanoTitleBar::Init(){
    this->setAttribute(Qt::WA_DeleteOnClose);
    //禁止父窗口影响子窗口样式
    this->setAttribute(Qt::WA_StyledBackground);
    //背景色
    this->setStyleSheet("background-color:rgb(64,64,64);");
    //开启鼠标追踪
    setMouseTracking(true);
    //为了实现鼠标的位置信息获取不受子控件的影响，启动鼠标悬浮追踪，代码如下：
    setAttribute(Qt::WA_Hover);
    setFixedHeight(40);





}

void KanoTitleBar::InitUI(){
    QHBoxLayout *hlaout=new QHBoxLayout(this);

    int w=20;
    int h=20;

    titleLabelText =new QLabel("Kano Player");
    QFont font=titleLabelText->font();
    font.setPointSize(20);
    font.setFamily("STCaiyun");
    titleLabelText->adjustSize();  //让QLabel自适应text的大小
    titleLabelText->setFont(font);
    titleLabelText->setFixedHeight(30);

//    titleLabelIcon = new QLabel(this);

//    titleLabelIcon->setFixedSize(w,h);
//    titleLabelIcon->setScaledContents(true);//使用它可以将QLabel中包含的图片在保持原始宽高比的基础上，自适应控件大小来展示图片
//    QImage image(":/images/icon/titleicon.png");
//    titleLabelIcon->setPixmap(QPixmap::fromImage(image));

    btnMax = new QPushButton(this);
    QIcon btnMaxicon(":/images/images/btnMax.png");
    btnMax->setFixedSize(w, h);
    btnMax->setIcon(btnMaxicon);


    btnMin = new QPushButton(this);
    btnMin->setFixedSize(w, h);
    QIcon btnMinicon(":/images/images/btnMin.png");
    btnMin->setIcon(btnMinicon);


    btnClose = new QPushButton(this);
    btnClose->setFixedSize(w, h);
    QSvgRenderer *svg_btnClose = new QSvgRenderer(QString(":/images/images/close.svg"));
    QPixmap *pixmap = new QPixmap(32, 32);
    pixmap->fill(Qt::transparent);
    //创建QPixmap 画布
    QPainter painter(pixmap);
    //将SVG图片写到画布中去
    svg_btnClose->render(&painter);
    QIcon ico(*pixmap);
    //按钮设置图标
    btnClose->setIcon(ico);

    hlaout->addWidget(titleLabelText);
   // hlaout->addWidget(titleLabelIcon);
    hlaout->addStretch();
    hlaout->addWidget(btnMin);
    hlaout->addSpacing(5);//间距
    hlaout->addWidget(btnMax);
    hlaout->addSpacing(5);//间距
    hlaout->addWidget(btnClose);
    setLayout(hlaout);

    btnMax->installEventFilter(this);
    btnMin->installEventFilter(this);
    btnClose->installEventFilter(this);

    connect(btnMax,&QPushButton::clicked,this,&KanoTitleBar::on_btnclicked);
    connect(btnMin,&QPushButton::clicked,this,&KanoTitleBar::on_btnclicked);
    connect(btnClose,&QPushButton::clicked,this,&KanoTitleBar::on_btnclicked);
}




void KanoTitleBar::RestoreUI(){
    int w = this->width();
    int h = this->height();
    btnClose->move(w - 21, 5);
    btnMax->move(w - 21 - 18 - 16, 5);
    btnMin->move(w - 21 - 34 * 2, 5);


}



void KanoTitleBar::on_btnclicked(){
    QPushButton *pButton = qobject_cast<QPushButton *>(sender());//sender,返回信号发送者的指针
    QWidget *pWindow = this->window();//返回此小部件的窗口，即返回窗口系统框架下一个祖先小部件//如果此部件就是窗口，则返回本身
    if (pWindow->isWindow())
    {
        if (pButton == btnMin)
        {
            pWindow->isMinimized() ? pWindow->showNormal() : pWindow->showMinimized();
        }
        else if (pButton == btnMax)
        {
            bool btnMaxbool=pWindow->isMaximized();
            //pWindow->isMaximized() ? pWindow->showNormal() : pWindow->showMaximized();
            emit btnMaxClicked(btnMaxbool);
        }
        else if (pButton == btnClose)
        {
            //pWindow->close();
            emit btnCloseSingnal();
        }
    }

}





//void KanoTitleBar::mousePressEvent(QMouseEvent *event)
//{
//    if (ReleaseCapture())
//    {
//        QWidget* pWindow = this->window();
//        if (pWindow->isWindow()&& pWindow->isMaximized())
//        {
//            SendMessage(HWND(pWindow->winId()), WM_SYSCOMMAND, SC_MOVE + HTCAPTION, 0);
//        }
//    }

//    event->ignore();

//}
void KanoTitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    QWidget *pWindow = this->window();
    if (pWindow->isWindow())
    {
        bool showmaxbool=pWindow->isMaximized();
        emit mouseDoubleClicked(!showmaxbool);
    }

}


bool KanoTitleBar::eventFilter(QObject *watched, QEvent *event)
{
    if((watched==btnMax||watched==btnClose||watched==btnMin)&&(event->type()==QEvent::KeyPress)){
        //屏蔽：因按钮被设置为焦点时，引起主窗口key_space事件被拦截的问题
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if ((keyEvent->key() == Qt::Key_Space)){
            event->ignore();
            return  true;
        }
    }
    return false;
}



