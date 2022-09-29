#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString picname = ("bug.png");
    QImage *img=new QImage;         //新建一个image对象
    if(!(img->load(picname))){
        ui->statusBar->showMessage(tr("Open Image Failed!"),3000);
        delete img;
        return;
    }
    else{
        ui->label->setPixmap(QPixmap::fromImage(*img));
        ui->statusBar->showMessage(tr("Open Image Success!"),3000); //成功时显示的内容
    }
}
