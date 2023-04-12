#-------------------------------------------------
#
# Project created by QtCreator 2023-04-11T22:13:21
#
#-------------------------------------------------

QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = testScreenshotApp
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    diffworker.cpp

HEADERS  += mainwindow.h \
    diffworker.h

FORMS    += mainwindow.ui
