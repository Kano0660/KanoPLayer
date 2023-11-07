#ifndef KANOTITLEBAR_H
#define KANOTITLEBAR_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>

class KanoTitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit KanoTitleBar(QWidget *parent = nullptr);

signals:
    void mouseDoubleClicked(bool showmaxbool);
    void btnMaxClicked(bool Maxbool);
    void btnCloseSingnal();
private:
    QPushButton * btnMax;
    QPushButton * btnMin;
    QPushButton * btnClose;
    QLabel * titleLabelIcon;
    QLabel * titleLabelText;
    bool isLeftPressed_title = false;

private:
    void Init();
    void InitUI();
    void RestoreUI();
    void on_btnclicked();

private slots:

    // QWidget interface
protected:
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

    // QWidget interface




    // QWidget interfac

    // QObject interface
public:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
};









#endif // KANOTITLEBAR_H
