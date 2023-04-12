#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace std;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), settings("YourCompany", "ScreenshotApp"), screenshotProcessActive(false)
{
    loadSettings();

    scrollArea = new QScrollArea(this);
    scrollArea->setGeometry(QRect(QPoint(10, 30), QSize(800, 400)));
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    gridWidget = new QWidget(scrollArea);
    gridLayout = new QGridLayout(gridWidget);
    gridLayout->setSpacing(5);
    gridWidget->setLayout(gridLayout);
    scrollArea->setWidget(gridWidget);

    startStopButton = new QPushButton("Start", this);
    startStopButton->setGeometry(QRect(QPoint(610, 430), QSize(200, 30)));
    connect(startStopButton, &QPushButton::clicked, this, &MainWindow::toggleScreenshotProcess);

    screenshotTimer = new QTimer(this);
    connect(screenshotTimer, &QTimer::timeout, this, &MainWindow::takeScreenshot);

    //Add an action to open the settings dialog
    QAction *settingsAction = new QAction("Settings", this);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettingsDialog);
    menuBar()->addAction(settingsAction);

    connect(&diffWorker, &DiffWorker::finished, this, &MainWindow::onDiffWorkerFinished);
    diffWorker.moveToThread(&diffWorkerThread);
    diffWorkerThread.start();

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("screenshots.db");

    if (!db.open())
    {
        QMessageBox::critical(this, "Error", "Failed to connect to the database");
    }

    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS screenshots (id INTEGER PRIMARY KEY, image BLOB, hash BLOB, difference REAL, timestamp DATETIME)"))
    {
        QMessageBox::critical(this, "Error", "Failed to create the screenshots table");
    }

    connect(&diffWorker, &DiffWorker::hashCalculated, this, &MainWindow::onHashCalculated);

    loadScreenshotsFromDatabase();
}

MainWindow::~MainWindow()
{
    diffWorkerThread.quit();
    diffWorkerThread.wait();
}

void MainWindow::loadSettings()
{
    settings.beginGroup("General");
    screenshotFolderPath = settings.value("screenshotFolderPath", QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).toString();
    screenshotFrequency = settings.value("screenshotFrequency", 60).toInt();
    settings.endGroup();
}

void MainWindow::saveSettings()
{
    settings.beginGroup("General");
    settings.value("screenshotFolderPath", screenshotFolderPath);
    settings.value("screenshotFrequency", screenshotFrequency);
    settings.endGroup();
}

void MainWindow::takeScreenshot()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QPixmap screenshot = screen->grabWindow(0);

    //Save screenshots to the database
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    screenshot.save(&buffer, "PNG");

    QSqlQuery query;
    query.prepare("INSERT INTO screenshots (image, timestamp) VALUES (:image, :timestamp)");
    query.bindValue(":image", byteArray);
    query.bindValue(":timestamp", QDateTime::currentDateTime());

    if (!query.exec())
    {
        QMessageBox::critical(this, "Error", "Failed to save the screenshot to the database");
    }

    //For local save without DB
    QString fileName = screenshotFolderPath + QDir::separator() + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png";
    screenshot.save(fileName);

    addScreenshotPreview(screenshot);
}

void MainWindow::openSettingsDialog()
{
    QString newFolder = QFileDialog::getExistingDirectory(this, "Select Screenshot Folder", screenshotFolderPath);
        if (!newFolder.isEmpty())
        {
            screenshotFolderPath = newFolder;
        }

        bool ok;
        int newFrequency = QInputDialog::getInt(this, "Screenshot Frequency", "Enter screenshot frequency (seconds):", screenshotFrequency, 1, 3600, 1, &ok);
        if (ok)
        {
            screenshotFrequency = newFrequency;
            screenshotTimer->setInterval(screenshotFrequency * 1000);
        }

        saveSettings();
}

void MainWindow::toggleScreenshotProcess()
{
    screenshotProcessActive = !screenshotProcessActive;
    if (screenshotProcessActive)
    {
        screenshotTimer->start(screenshotFrequency * 1000);
        screenshotProcessActive = true;
        updateButtonState("Stop");
    }
    else
    {
        screenshotTimer->stop();
        screenshotProcessActive = false;
        updateButtonState("Start");
    }
}

void MainWindow::updateButtonState(QString buttonState)
{
    startStopButton->setText(buttonState);
}

void MainWindow::addScreenshotPreview(const QPixmap &screenshot)
{
    QWidget *previewWidget = new QWidget(gridWidget);
    QVBoxLayout *vbox = new QVBoxLayout(previewWidget);

    QLabel *preview = new QLabel(previewWidget);
    preview->setPixmap(screenshot.scaled(QSize(200, 200), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    vbox->addWidget(preview);

    if (!screenshotWidgets.isEmpty())
    {
        QLabel *compareLabel = new QLabel("Calculating...", previewWidget);
        compareLabel->setObjectName("compareLabel");
        vbox->addWidget(compareLabel);

        QPixmap previousScreenshot = *screenshotWidgets.first()->findChild<QLabel*>()->pixmap();
        diffWorker.process(screenshot, previousScreenshot);
    }
    else
    {
        QLabel *compareLabel = new QLabel(QString("Difference: 0%"), previewWidget);
        vbox->addWidget(compareLabel);
    }

    screenshotWidgets.prepend(previewWidget);

    for (int i = 0; i < screenshotWidgets.size(); ++i)
    {
        int row = i / 3;
        int column = i % 3;
        gridLayout->addWidget(screenshotWidgets[i], row, column);
    }

    if (screenshotWidgets.size() > 20)
    {
        scrollArea->setWidgetResizable(false);
        gridWidget->setMinimumSize(QSize(600, (screenshotWidgets.size() / 3) * 130));
    }
}

void MainWindow::onDiffWorkerFinished(double differencePercentage)
{
    QLabel *compareLabel = screenshotWidgets.first()->findChild<QLabel*>("compareLabel");
    if (compareLabel)
    {
        compareLabel->setText(QString("Difference: %1%").arg(differencePercentage, 0, 'f', 2));
    }

    //Store the difference percentage in the DB
    QSqlQuery query;
    query.prepare("UPDATE screenshots SET difference = :difference WHERE id = (SELECT MAX(id) FROM screenshots)");
    query.bindValue(":difference", differencePercentage);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Error", "Failed to save the difference between the screenshots");
    }
}

//Storing the hash in the DB
void MainWindow::onHashCalculated(const QByteArray &hash)
{
    QSqlQuery query;
    query.prepare("UPDATE screenshots SET hash = :hash WHERE id = (SELECT MAX(id) FROM screenshots)");
    query.bindValue(":hash", hash);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Error", "Failed to save the hash sum in the database");
    }
}

void MainWindow::loadScreenshotsFromDatabase()
{
    QSqlQuery query("SELECT image, hash, difference, timestamp FROM screenshots ORDER BY timestamp DESC");

    while (query.next())
    {
        QPixmap screenshot;
        QByteArray byteArray = query.value(0).toByteArray();
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::ReadOnly);
        screenshot.loadFromData(buffer.readAll(), "PNG");

        addScreenshotPreview(screenshot);

        QByteArray hash = query.value(1).toByteArray();
        double differencePercentage = query.value(2).toDouble();

        QLabel *compareLabel = screenshotWidgets.first()->findChild<QLabel*>("compareLabel");
        if (compareLabel)
        {
            if (!hash.isEmpty())
            {
                compareLabel->setText(QString("Difference: %1%").arg(differencePercentage, 0, 'f', 2));
            }
            else
            {
                compareLabel->setText("Hosh not available");
            }
        }
    }
}
