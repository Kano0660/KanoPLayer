#ifndef WIDGET_H
#define WIDGET_H
#include "kanowidget.h"
#include <QWidget>


class Widget : public KanoWidget
{
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = nullptr);

signals:

};

#endif // WIDGET_H
