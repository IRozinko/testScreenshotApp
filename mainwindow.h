#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QScreen>
#include <QStandardPaths>
#include <QFileDialog>
#include <QSettings>
#include <QGuiApplication>
#include <QPixmap>
#include <QDateTime>
#include <QDir>
#include <QInputDialog>
#include <QPushButton>
#include <QWidget>
#include <QVector>
#include <QScrollArea>
#include <QLabel>
#include <QGridLayout>
#include <QThread>
#include <QtSql/QSqlDatabase>
#include <QMessageBox>
#include <QBuffer>
#include <QtSql/QSqlQuery>
#include "diffworker.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void takeScreenshot();
    void openSettingsDialog();
    void toggleScreenshotProcess();
    void onDiffWorkerFinished(double differencePercentage);
    void onHashCalculated(const QByteArray &hash);

private:
    Ui::MainWindow *ui;
    QString screenshotFolderPath;
    QTimer *screenshotTimer;
    QSettings settings;
    QPushButton *startStopButton;
    QScrollArea *scrollArea;
    QWidget *gridWidget;
    QGridLayout *gridLayout;
    QVector<QWidget*> screenshotWidgets;
    QThread diffWorkerThread;
    DiffWorker diffWorker;
    QSqlDatabase db;

    int screenshotFrequency;

    bool screenshotProcessActive;

    void loadSettings();
    void saveSettings();
    void updateButtonState(QString buttonState);
    void addScreenshotPreview(const QPixmap &screenshot);
    void loadScreenshotsFromDatabase();
};

#endif // MAINWINDOW_H
