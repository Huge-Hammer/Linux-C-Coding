#include "video_show.h"

videoshow::videoshow(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
}

void videoshow::paintEvent(QPaintEvent *){
    try{
        QPainter painter(this);

        if(!img.isNull()){
            painter.drawImage(QPointF(0,0),img);
        }

    }catch(...){}
}
