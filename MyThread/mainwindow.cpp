#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //实例化线程
    t = new mythread(this);
    t->getLabel(ui->label);
}

//关闭窗口时结束线程
MainWindow::~MainWindow()
{
    qDebug("thread exit");
    t->terminate();
    delete ui;
}

void MainWindow::on_openbt_clicked()
{
    t->start();
}

void MainWindow::on_closebt_clicked()
{
    t->terminate();
}
