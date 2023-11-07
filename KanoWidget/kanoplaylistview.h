#ifndef KANOPLAYLISTVIEW_H
#define KANOPLAYLISTVIEW_H

#include <QListView>
#include <QWidget>

class KanoPlayListView : public QListView
{
    Q_OBJECT
public:
    explicit KanoPlayListView(QWidget *parent = nullptr);

    //函数获取用户选择的项的索引列表。
    QModelIndexList getSelectedIndexes();
};

#endif // KANOPLAYLISTVIEW_H
