#ifndef VIDEOSHOW_H
#define VIDEOSHOW_H

#include <QWidget>
#include <QPainter>
#include <QPointF>

class videoshow : public QWidget{
    Q_OBJECT

public:
    explicit videoshow(QWidget *parent = 0);

    /* 显示的每帧图像 */
    QImage img;

    /* 重写父类下的protected方法*/
protected:
    void paintEvent(QPaintEvent *);
};

#endif // VIDEOSHOW_H
