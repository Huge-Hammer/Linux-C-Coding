#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QThread>
#include <QWidget>
#include <QLabel>
#include <QDebug>

class mythread : public QThread
{
    Q_OBJECT
public:
    explicit mythread(QWidget *parent = nullptr);
    ~mythread(){}

    //重写run方法
    void run();

    //定义一个公有方法接收传递过来的参数--》标签
    void getLabel(QLabel *lb);

private:
    QLabel *mylb;
    QStringList piclist;
};
#endif // MYTHREAD_H
