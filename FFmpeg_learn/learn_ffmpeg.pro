#-------------------------------------------------
#
# Project created by QtCreator 2022-07-20T14:40:12
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = learn_ffmpeg
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

if(contains(DEFINES,PLATFORM_X86)){
    message("compile for x86")
    INCLUDEPATH += /home/hammer/ffmpeg-hub/x86-ffmpeg/x86-install/ffmpeg5.0/include
    LIBS +=  -L/home/hammer/ffmpeg-hub/x86-ffmpeg/x86-install/ffmpeg5.0/lib  -lavcodec   -lavdevice  -lavfilter  -lavformat -lavutil -lswresample -lswscale  -lm   -lpthread
}else{
    message("compile for arm")
    INCLUDEPATH += /home/hammer/ffmpeg-hub/arm-ffmpeg/arm-install/ffmpeg5.0/include
    LIBS +=  -L/home/hammer/ffmpeg-hub/arm-ffmpeg/arm-install/ffmpeg5.0/lib  -lavcodec   -lavdevice  -lavfilter  -lavformat -lavutil -lswresample -lswscale  -lm   -lpthread
}



SOURCES += \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h

FORMS += \
        mainwindow.ui
