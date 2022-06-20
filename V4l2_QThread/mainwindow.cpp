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

    exitBtn      = new QPushButton("退出",this);
    startCapture = new QPushButton("开始",this);
    takePic      = new QPushButton("拍照",this);

    video = new videoshow();
    video->setFixedWidth(PIXWIDTH);
    video->setFixedHeight(PIXHEIGHT);

    /* 按钮属性 */
    exitBtn->setMaximumHeight(40);
    exitBtn->setMaximumWidth(100);
    startCapture->setMaximumHeight(40);
    startCapture->setMaximumWidth(100);
    takePic->setMaximumHeight(40);
    takePic->setMaximumWidth(100);
    takePic->setEnabled(false);

    vboxLayout->addWidget(startCapture);
    vboxLayout->addWidget(takePic);
    vboxLayout->addWidget(exitBtn);

    rightWidget->setLayout(vboxLayout);

    hboxLayout->addWidget(video);
    hboxLayout->addWidget(rightWidget);

    mainWidget->setLayout(hboxLayout);
    this->setCentralWidget(mainWidget);

    /* 信号连接 */
    connect(exitBtn,SIGNAL(clicked()),this,SLOT(onExitClicked()));
    connect(startCapture,SIGNAL(clicked()),this,SLOT(onStartCapture()));
    connect(takePic,SIGNAL(clicked()),this,SLOT(onTakePic()));

    t = new V4l2Thread(video);

    pic_num = 1;

    dir = new QDir();
    if(!dir->exists("images")){
        dir->mkdir("images");
    }
    dir->setCurrent("images");
}

MainWindow::~MainWindow()
{

}

void MainWindow::onExitClicked()
{
    t->show_flag = false;
    t->exit_show();
    t->destroyed();
    qDebug("bye bye");
    this->close();
    exit(0);
}

void MainWindow::onStartCapture()
{
    if(t->show_flag == false){
        t->show_flag = true;
        t->start();
        takePic->setEnabled(true);
        qDebug("start");
    }
}

void MainWindow::onTakePic()
{
    if(!video->img.isNull()){
        QString fileName = QCoreApplication::applicationDirPath() + "/images" + "/test" + QString::number(pic_num) + ".jpeg";
        qDebug()<<"正在保存"<<fileName<<"图片，请稍后..."<<endl;
        video->img.save(fileName, "JPEG", -1);
        qDebug()<<"保存完成！ "<<endl;
        pic_num++;
    }
}
