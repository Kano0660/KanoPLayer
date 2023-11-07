#ifndef WIDGET_HF
#define WIDGET_HF
//检查您是否在 2 个不同的标头（即 #ifndef X_H 和 #define X_H 行）中意外获得了 2 个同名的包含守卫

//（根据您的描述，这可能不太可能：如果错误发生在 moc_Y.cpp 的编译中，则更有可能）
#include <QWidget>
#include <kanotitlebar.h>//标题栏
#include "kanoplaywidget.h"//播放器
#include "kanoplayctrlbar.h"//控制栏
#include "kanoopenfile.h" //打开文件按钮
#include "kanolistviewwidget.h"//播放列表
#include <mutex>


extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavformat/version.h>
}




//widght区域分块
enum Location{
    TOP,
    BOTTOM,
    LEFT,
    RIGHT,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    CENTER
};

struct PlayerLayoutHeight{
    int title=40;
    int player=480;
    int playerctr=80;

};

class KanoWidget : public QWidget
{
    Q_OBJECT

public:
    KanoWidget(QWidget *parent = nullptr);
    ~KanoWidget();



protected:

    bool playstatus =false;//当前是否有视频在播放
    bool playstatus_keyspace =false;
    bool isLeftPressed;
    bool isLeftPressed_Move=false;//标题栏被点击触发的move
    bool isLeftPressed_Move_title=false;//标题栏被点击触发的move后，具体的窗口移动
    QPoint mouseOFFset;//窗口与鼠标偏移
    QPoint mouseOFFset_T;//点击标题栏，缩小窗口后：窗口会移动到moveX,moveY;记录鼠标点击位置与新窗口的偏移
    Location location;
    QRect Standardrect;


    int moveX = 0;//缩小窗口的新x坐标
    int moveY = 0;//缩小窗口的新y坐标

    PlayerLayoutHeight LayoutHeight;


private:
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout_2;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    KanoTitleBar *kanoTitleBar;
    KanoPlayWidget *kanoPlayWidget;
    KanoPlayCtrlBar *kanoPlayCtrlBar;
    KanoOpenFile   *kanoOpenFile;
    KanoListViewWidget *kanoListViewWidget;

    QPushButton *btnList;
    QPushButton *btnPlayStatus;

    const QString m_vFmt;//媒体格式
    QString playFileUrl;
    QStringList playFileUrlList;
    QString currentPlayFileUrl;//当前播放视频

    bool isFullScreenBool =false;
    bool isMaximizedBool =false;



    QTimer *m_timer;

signals:   
    void openfile(const QString playurl);


private slots:
    void m_OpenFileclicked();
    void slot_playhide(bool checked);
    void do_timer_timeout();
    void playListViewDClickSlot(const QModelIndex &index);
    void onplay_mouseClicked();
    void on_btnList(bool checked = false);
    void on_btnPlayStatus(bool checked = false);
    void on_playNext();
    void on_playUpward();
    void on_ShowList();
private:
    void InitUI();
    void Kano();
    void InitList();
    void Layout();
    void UpdateUI();
    void setCursorShape(const QPoint &globalPos);



    void addWidget(QWidget);

    void EntershowFullScreen();
    void LeaveshowFullScreen();
    void onFullScreen_showPlayCtrBar();

    void EntershowMaximized();
    void LeaveshowMaximized();
    void CursorShahp(const QPoint &globalPos);


    // QWidget interface
protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

public:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
};




#endif // WIDGET_H
