#include "kanolistviewwidget.h"
#include <QListView>
#include <QStringListModel>
#include <QItemSelectionModel>
#include <QSettings>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QMenu>
#include <QEvent>


KanoListViewWidget::KanoListViewWidget(QWidget *parent)
    : QWidget{parent}
{
    setFixedWidth(200);
    //开启鼠标追踪
    setMouseTracking(true);
    //为了实现鼠标的位置信息获取不受子控件的影响，启动鼠标悬浮追踪，代码如下：
    setAttribute(Qt::WA_Hover);

    controlWidget =new QWidget(this);
    controlWidget->setFixedHeight(20);

    //显示播放列表
    playListView = new KanoPlayListView(controlWidget);
    playListView->setObjectName("playListView");
    playListView->setEditTriggers(QAbstractItemView::NoEditTriggers);//不允许编辑
    playListView->setSelectionMode(QAbstractItemView::ExtendedSelection);//按住ctrl多选



    btnAddList = new QPushButton(controlWidget);
    QPixmap pixmap = QPixmap(":/images/images/btnAddList.svg");
    btnAddList->setIcon(pixmap);
    btnAddList->setIconSize(QSize(20, 20));
    QPainter p(btnAddList);
    //表示引擎应尽可能对图元的边缘进行抗锯齿。
    p.setRenderHint(QPainter::Antialiasing);


    btnClear =new QPushButton(controlWidget);
    QPixmap pixmap2 = QPixmap(":/images/images/btnClear.svg");
    btnClear->setIcon(pixmap2);
    btnClear->setIconSize(QSize(20, 20));
    QPainter p2(btnClear);
    //表示引擎应尽可能对图元的边缘进行抗锯齿。
    p2.setRenderHint(QPainter::Antialiasing);

    QMenu* pMenu = new QMenu(controlWidget);
    QAction* pAc1 = new QAction(tr("清空列表"), this);
    QAction* pAc2 = new QAction(tr("删除选中"), this);
    pMenu->addAction(pAc1);
    pMenu->addAction(pAc2);

    btnClear->setMenu(pMenu);



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

    playListLabel =new QLabel(controlWidget);
    playListLabel->setText("播放列表");
    playListLabel->setFixedSize(60,20);
    playListLabel->setStyleSheet("QLabel {background-color:rgb(100,100,100);}");
    QFont font=playListLabel->font();
    font.setPointSize(10);
    playListLabel->adjustSize();  //让QLabel自适应text的大小
    playListLabel->setFont(font);



    Layout();


    InitList();
    playListView->installEventFilter(this);



    connect(playListView,&QListView::doubleClicked,this,&KanoListViewWidget::onview_doubleClicked);
    connect(btnAddList,&QPushButton::clicked,this,&KanoListViewWidget::on_btnAddListClicked);

    connect(pAc1, &QAction::triggered,this,&KanoListViewWidget::on_clearList);
    connect(pAc2, &QAction::triggered,this,&KanoListViewWidget::on_deleteList);



}


void KanoListViewWidget::Layout(){

    //控制栏widget
    QGridLayout * gridLayout2=new QGridLayout(controlWidget);
    gridLayout2->setSpacing(0);
    gridLayout2->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout * horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(playListLabel);

    QSpacerItem *horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addItem(horizontalSpacer);
    horizontalLayout->addWidget(btnAddList);
    horizontalLayout->addWidget(btnClear);
    gridLayout2->addLayout(horizontalLayout, 0, 0, 1, 1);



    //整个列表
    QGridLayout * gridLayout=new QGridLayout(this);
    gridLayout->setSpacing(0);
    gridLayout->setContentsMargins(0, 0, 0, 0);


    QVBoxLayout * verticalLayout = new QVBoxLayout();
    verticalLayout->setSpacing(0);
    verticalLayout->setContentsMargins(0,0,0,0);

    verticalLayout->addWidget(controlWidget);
    verticalLayout->addWidget(playListView);
    gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);




}

void KanoListViewWidget::setPlayListToSettings()
{
    QSettings settings;
    if(playListModel->rowCount()!=0){
        playList =playListModel->stringList();
        settings.setValue("playList",playList);
    }
    else{
        settings.setValue("playList","0");//当本次列表被清空，往注册表中写"0"
    }
}

void KanoListViewWidget::OpenFiles(QStringList UrlList)
{
    int playnum =playListModel->rowCount();

    for(auto list:UrlList){
        if(playnum == 0){//当前列表是否为空
            playListModel->insertRow(playListModel->rowCount());
            QModelIndex index = playListModel->index(playListModel->rowCount()-1);
            playListModel->setData(index,list,Qt::DisplayRole);
            continue;
        }
        for(int i=1;i<=playnum;i++){
            if(list == playListModel->data(playListModel->index(i-1),Qt::DisplayRole).toString()){
                break;
            }
            if(i==playnum){
                playListModel->insertRow(playListModel->rowCount());
                QModelIndex index = playListModel->index(playListModel->rowCount()-1);
                playListModel->setData(index,list,Qt::DisplayRole);
            }
        }
    }
}

int KanoListViewWidget::getUrlSerial(QString Url)
{
    if(playListModel->rowCount()<=0)
        return -1;
    int i = 0;
    for(auto list:playListModel->stringList()){
        if(list==Url)
            break;
        i++;
    }
    return i;
}

int KanoListViewWidget::getPlayListNumber()
{
    return playListModel->rowCount();
}

QString KanoListViewWidget::getUrl(int row)
{
    return playListModel->index(row).data(Qt::DisplayRole).toString();
}

void KanoListViewWidget::setCurrentIndex(int row)
{
    QModelIndex index=playListModel->index(row);
    playListView->setCurrentIndex(index);
}



void KanoListViewWidget::InitList(){

    playListModel =new QStringListModel(this);
    playListView->setModel(playListModel);

    QSettings settings;
    bool checked = settings.contains("playList");


    //settings.remove("playList");
    //第一次打开应用注册playList
    if(!checked){
        settings.setValue("playList","0");
        playList={"0"};
    }
    else{
        playList= settings.value("playList").toStringList();
        if(playList.at(0)!="0"){
            playListModel->setStringList(playList);
        }
    }


}

void KanoListViewWidget::onview_doubleClicked(const QModelIndex &index)
{
    emit doubleClicked(index);
}

void KanoListViewWidget::on_btnAddListClicked(bool checked)
{
    Q_UNUSED(checked)
    emit btnAddListClick();
}

void KanoListViewWidget::on_clearList()
{
    playListModel->removeRows(0,playListModel->rowCount());
}

#include <QMessageBox>
void KanoListViewWidget::on_deleteList()
{
    //存在问题:SelectedIndexes保存选中项的具体顺序，
    //即对于如右7行:1,2,3,4,5,6,7;
    //非顺序选择:如果先后选择7,1,6行，SelectedIndexes保存的索引就是6,0,5;因此需要设置缓冲区保存选择顺序索引

    std::vector<int> buffer;
    buffer.resize(258,0);

    QModelIndexList modelIndexList=playListView->getSelectedIndexes();

    for(auto list:modelIndexList){
        buffer[list.row()]=list.row()+1;
    }

    if(playListModel->rowCount()>258){
        QMessageBox::warning(this,"列表容量","列表容量超过缓冲区大小,已进行重新分配");
        buffer.resize(playListModel->rowCount(),0);

    }
    int deletenum=0;//已经删除的数量
    for(int i=0;i<buffer.size()-1;i++){
        if(buffer[i]!=0){
            //虽然保存选择顺序，但是删除时按顺序删除，即只要原列表中的项只要被选中，就按顺序删除
            playListModel->removeRow(i-deletenum);
            deletenum++;
        }
    }

}


