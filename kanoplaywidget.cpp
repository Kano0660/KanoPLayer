#include "kanoplaywidget.h"

#include <QGridLayout>

KanoPlayWidget::KanoPlayWidget(QWidget *parent)
    : QWidget{parent}
{
    Init();
}

void KanoPlayWidget::Init()
{

    resize(800,480);
    this->setAttribute(Qt::WA_DeleteOnClose);
    //禁止父窗口影响子窗口样式
    this->setAttribute(Qt::WA_StyledBackground);
    //背景色
    this->setStyleSheet("background-color:rgb(0,0,0);");
    //开启鼠标追踪
    setMouseTracking(true);
    //为了实现鼠标的位置信息获取不受子控件的影响，启动鼠标悬浮追踪，代码如下：
    setAttribute(Qt::WA_Hover);
    kanoOpenGLWidget =new KanoOpenGLWidget(this);

    QGridLayout * gridLayout=new QGridLayout(this);
    gridLayout->setSpacing(0);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->addWidget(kanoOpenGLWidget, 0, 0, 1, 1);
    setRatio(16,9);
    //自定义数据类型在槽中作为参数传递需先注册
    qRegisterMetaType<QSharedPointer<YUV420Frame>>("QSharedPointer<YUV420Frame>");
    connect(this,&KanoPlayWidget::onShowYUVToOpenGL,kanoOpenGLWidget,&KanoOpenGLWidget::onShowYUV);
    connect(kanoOpenGLWidget,&KanoOpenGLWidget::mouseDoubleClicked,this,&KanoPlayWidget::openglmouseDoubleClicked);
    connect(kanoOpenGLWidget,&KanoOpenGLWidget::mouseClicked,this,&KanoPlayWidget::openglmouseClicked);
}



void KanoPlayWidget::resizeEvent(QResizeEvent *event)
{
    setRatio(RatioX,RatioY);
    QWidget::resizeEvent(event);

}


void KanoPlayWidget::setRatio(int x, int y)
{
    RatioX = x;
    RatioY = y;

    int tempwidth=this->width();
    int tempheight=this->height();
    float ratio=(float)x/y;
    float tempy=tempwidth/ratio;
    float tempx=tempheight*ratio;
    if((tempheight*ratio)>=tempwidth){
        kanoOpenGLWidget->setGeometry(0,(tempheight-tempy)*0.5,tempwidth,tempy);
        //kanoOpenGLWidget->move();
        update();
    }
    else{
        kanoOpenGLWidget->setGeometry((tempwidth-tempx)*0.5,0,tempx,tempheight);
         update();
    }

}

void KanoPlayWidget::onShowYUV(QSharedPointer<YUV420Frame> frame)
{
    emit onShowYUVToOpenGL(frame);
}

void KanoPlayWidget::openglmouseDoubleClicked()
{
    emit mouseDoubleClicked();
}

void KanoPlayWidget::openglmouseClicked()
{
    emit mouseClicked();
}
void KanoPlayWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton){
        emit mouseClicked();
    }

}

void KanoPlayWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    emit mouseDoubleClicked();
}
