#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mythread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_openbt_clicked();

    void on_closebt_clicked();

private:
    Ui::MainWindow *ui;
    mythread *t;
};

#endif // MAINWINDOW_H
