#include "kanoopenfile.h"

KanoOpenFile::KanoOpenFile(QWidget *parent)
    :QToolButton(parent)
{
    setText("选择文件");
    setObjectName("kanoopenfile");
    setFixedSize(70,30);
}
