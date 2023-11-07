#include "kanoplaylistview.h"

KanoPlayListView::KanoPlayListView(QWidget *parent):
    QListView(parent)
{

}

QModelIndexList KanoPlayListView::getSelectedIndexes()
{
    return this->selectedIndexes();
}
