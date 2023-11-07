#ifndef KANOLISTVIEWWIDGET_H
#define KANOLISTVIEWWIDGET_H

#include <QWidget>
#include "KanoWidget/kanoplaylistview.h"
class QListView;
class QStringListModel;
class QItemSelectionModel;
class QPushButton;
class QLabel;
class KanoListViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KanoListViewWidget(QWidget *parent = nullptr);


public:
    void setPlayListToSettings();
    void OpenFiles(QStringList UrlList);
    //获取url对应的url索引
    int getUrlSerial(QString Url);
    //获取当前列表item个数
    int getPlayListNumber();
    //获取url路径
    QString getUrl(int row);
    //设置当前选中项
    void setCurrentIndex(int row);



private:
    //显示播放列表
    KanoPlayListView *playListView;
    QStringListModel *playListModel;
    QItemSelectionModel *playListSelectionModel;
    QStringList playList;

    QWidget *controlWidget;
    QPushButton *btnAddList;
    QPushButton *btnClear;//删除选中 or 清空列表
    QLabel *playListLabel;

private:
    void InitList();
    void Layout();

signals:
    void doubleClicked(const QModelIndex &index);
    void btnAddListClick();


private slots:
    void onview_doubleClicked(const QModelIndex &index);
    void on_btnAddListClicked(bool checked = false);

    void on_clearList();
    void on_deleteList();
};


#endif // KANOLISTVIEWWIDGET_H
