#ifndef KANOPLAYWIDGET_H
#define KANOPLAYWIDGET_H
#include "kanoopenglwidget.h"
#include <KanoPlayer.h>
#include <QWidget>
#include <QMouseEvent>
class KanoPlayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KanoPlayWidget(QWidget *parent = nullptr);

public:
    void Init();
    void setRatio(int x,int y);//视频比例

private:
    KanoOpenGLWidget *kanoOpenGLWidget;
    int RatioX =16;
    int RatioY =9;

signals:
    void mouseDoubleClicked();
    void mouseClicked();
    void onShowYUVToOpenGL(QSharedPointer<YUV420Frame> frame);
public slots:
    void onShowYUV(QSharedPointer<YUV420Frame> frame);
private slots:
    void openglmouseDoubleClicked();
    void openglmouseClicked();

    // QWidget interface
protected:
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

    // QWidget interface
protected:
    virtual void mousePressEvent(QMouseEvent *event) override;

    // QWidget interface
protected:
    virtual void resizeEvent(QResizeEvent *event) override;
};


#endif // KANOPLAYWIDGET_H
