#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    /* 获取屏幕的分辨率，Qt官方建议使用这种方法获取屏幕分辨率，防上多屏设备导致对应不上 */
    QList <QScreen *> list_screen =  QGuiApplication::screens();

    /* 如果是ARM平台，直接设置大小为屏幕的大小 */
#if __arm__
    /* 重设大小 */
    this->resize(list_screen.at(0)->geometry().width(),
        list_screen.at(0)->geometry().height());
#else
    /* 否则则设置主窗体大小为800x480 */
    this->resize(800, 480);
#endif

    /* 控件实例化 */
    mainWidget  = new QWidget();
    rightWidget = new QWidget();

    hboxLayout  = new QHBoxLayout();
    vboxLayout = new QVBoxLayout();


    startBtn = new QPushButton("开始",this);
    capBtn   = new QPushButton("拍照",this);
    recBtn   = new QPushButton("录制",this);

    video = new videoshow();
    video->setFixedWidth(PIXWIDTH);
    video->setFixedHeight(PIXHEIGHT);

    /* 按钮属性 */
    recBtn->setMaximumHeight(40);
    recBtn->setMaximumWidth(100);
    startBtn->setMaximumHeight(40);
    startBtn->setMaximumWidth(100);
    capBtn->setMaximumHeight(40);
    capBtn->setMaximumWidth(100);
    capBtn->setEnabled(false);

    vboxLayout->addWidget(startBtn);
    vboxLayout->addWidget(capBtn);
    vboxLayout->addWidget(recBtn);

    rightWidget->setLayout(vboxLayout);

    hboxLayout->addWidget(video);
    hboxLayout->addWidget(rightWidget);

    mainWidget->setLayout(hboxLayout);
    this->setCentralWidget(mainWidget);

    /* 信号连接 */
    connect(startBtn,SIGNAL(clicked()),this,SLOT(onStart()));
    connect(capBtn,SIGNAL(clicked()),this,SLOT(onCap()));
    connect(recBtn,SIGNAL(clicked()),this,SLOT(onRec()));

    /* QThread inst */
    t = new YUYVQThread(video);

    dir = new QDir();
    if(!dir->exists("images")){
        dir->mkdir("images");
    }
    dir->setCurrent("images");
}

MainWindow::~MainWindow()
{

}

//窗口关闭，进程停止
void MainWindow::closeEvent(QCloseEvent *event)
{
    t->show_flag = false;
    t->wait();
    event->accept();
    qDebug("thread exit");
}

void MainWindow::onStart()
{
    if(t->show_flag == false){
        t->show_flag = true;
        t->start();
        capBtn->setEnabled(true);
        qDebug("start");
    }
}

void MainWindow::onCap()
{
    QDateTime ntime = QDateTime::currentDateTime();
    QString dts = ntime.toString("yyMMddHHmmss");
    if(!video->img.isNull()){
        QString fileName = QCoreApplication::applicationDirPath() + "/images/" + dts + ".png";
        qDebug()<<"正在保存"<<fileName<<"图片，请稍后..."<<endl;
        video->img.save(fileName, "PNG", -1);
        qDebug()<<"保存完成！ "<<endl;
    }
}

void MainWindow::onRec()
{
    qDebug("开始录制");
}
