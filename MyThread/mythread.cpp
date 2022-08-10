#include "mythread.h"

mythread::mythread(QWidget *parent) : QThread(parent)
{
    piclist.append("://images/head.jpg");
    piclist.append("://images/LOGO_BX.png");
    piclist.append("://images/OIP-C.jpg");
    piclist.append("://images/yan.png");
}

void mythread::run()
{
    int n=0;
    while (1) {
        QPixmap mymap(piclist.at(n));
        mymap.scaled(mylb->width(),mylb->height());
        mylb->setScaledContents(true);
        mylb->setPixmap(mymap);

        if(n<3)
             n++;
        else
             n=0;

        QThread::sleep(1);
    }
}

void mythread::getLabel(QLabel *lb)
{
    mylb=lb;
}
