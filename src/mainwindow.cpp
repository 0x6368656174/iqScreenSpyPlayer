#define SETTINGS_HOSTS "hosts"

#define SETTINGS_MAIN_WINDOW_GROUP "mainWindowSettings"
#define SETTINGS_MAIN_WINDOW_STATE "state"
#define SETTINGS_MAIN_WINDOW_GEOMETRY "geometry"
#define SETTINGS_MAIN_WINDOW_START "startDateTime"
#define SETTINGS_MAIN_WINDOW_END "endDateTime"
#define SETTINGS_MAIN_WINDOW_LAST_SAVE_DIR "lastSaveDir"

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_mediaPlayer(new QMediaPlayer(this)),
    m_mediaPlaylist(new QMediaPlaylist(m_mediaPlayer)),
    m_videoProbe(new QVideoProbe(this))
{
    ui->setupUi(this);

    ui->videoWidget->setAspectRatioMode(Qt::KeepAspectRatio);

    m_mediaPlayer->setVideoOutput(ui->videoWidget);
    m_mediaPlayer->setPlaylist(m_mediaPlaylist);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged,
            ui->positionSlider, &QSlider::setMaximum);
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged,
            ui->positionSlider, &QSlider::setValue);
    connect(m_mediaPlayer, &QMediaPlayer::stateChanged,
            this, &MainWindow::changeIntefaceOnMediaStateChanged);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged,
            this, &MainWindow::changeTimeLabelText);
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged,
            this, &MainWindow::changeTimeLabelText);

    m_videoProbe->setSource(m_mediaPlayer);
    connect(m_videoProbe, &QVideoProbe::videoFrameProbed,
            this, &MainWindow::saveLastFrame);

    connect(ui->positionSlider, &QSlider::valueChanged,
            m_mediaPlayer, &QMediaPlayer::setPosition);
    connect(ui->playPushButton, &QPushButton::clicked,
            this, &MainWindow::playPause);
    connect(ui->stopPushButton, &QPushButton::clicked,
            m_mediaPlayer, &QMediaPlayer::stop);
    connect(ui->screenshotPushButton, &QPushButton::clicked,
            this, &MainWindow::saveCurrentFrame);


    loadSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initSettings() const
{
    QSettings settings;
    if (!settings.contains(SETTINGS_HOSTS))
        settings.setValue(SETTINGS_HOSTS, QStringList());
}

void MainWindow::loadSettings()
{
    initSettings();

    QSettings settings;
    ui->hostComboBox->addItems(settings.value(SETTINGS_HOSTS).toStringList());

    settings.beginGroup(SETTINGS_MAIN_WINDOW_GROUP);
    ui->startDateTimeEdit->setDateTime(settings.value(SETTINGS_MAIN_WINDOW_START, QDateTime::currentDateTime()).toDateTime());
    ui->endDateTimeEdit->setDateTime(settings.value(SETTINGS_MAIN_WINDOW_END, QDateTime::currentDateTime()).toDateTime());
    restoreGeometry(settings.value(SETTINGS_MAIN_WINDOW_GEOMETRY).toByteArray());
    restoreState(settings.value(SETTINGS_MAIN_WINDOW_STATE).toByteArray());
    settings.endGroup();
}

void MainWindow::saveSettings() const
{
    QSettings settings;
    settings.beginGroup(SETTINGS_MAIN_WINDOW_GROUP);
    settings.setValue(SETTINGS_MAIN_WINDOW_START, ui->startDateTimeEdit->dateTime());
    settings.setValue(SETTINGS_MAIN_WINDOW_END, ui->endDateTimeEdit->dateTime());
    settings.setValue(SETTINGS_MAIN_WINDOW_GEOMETRY, saveGeometry());
    settings.setValue(SETTINGS_MAIN_WINDOW_STATE, saveState());
    settings.endGroup();
}

void MainWindow::changeIntefaceOnMediaStateChanged() const
{
    switch (m_mediaPlayer->state()) {
    case QMediaPlayer::PlayingState:
        ui->playPushButton->setIcon(QIcon("://icons/control-pause.png"));
        ui->stopPushButton->setEnabled(true);
        ui->positionSlider->setEnabled(true);
        ui->timeLabel->setEnabled(true);
        ui->screenshotPushButton->setEnabled(true);
        break;
    case QMediaPlayer::PausedState:
        ui->playPushButton->setIcon(QIcon("://icons/control.png"));
        ui->stopPushButton->setEnabled(true);
        ui->positionSlider->setEnabled(true);
        ui->timeLabel->setEnabled(true);
        ui->screenshotPushButton->setEnabled(true);
        break;
    case QMediaPlayer::StoppedState:
        ui->playPushButton->setIcon(QIcon("://icons/control.png"));
        ui->stopPushButton->setEnabled(false);
        ui->positionSlider->setEnabled(false);
        ui->timeLabel->setEnabled(false);
        ui->screenshotPushButton->setEnabled(false);
        break;
    }
}

void MainWindow::playPause() const
{
    if (m_mediaPlayer->state() == QMediaPlayer::PlayingState)
        m_mediaPlayer->pause();
    else
        playFile(QUrl::fromLocalFile("/home/pashok/vboxshare/sun27_15.07.2015_09.10-15.07.2015_09.20.avi"));
}

void MainWindow::changeTimeLabelText() const
{
    ui->timeLabel->setText(QString("%0/%1")
                           .arg(QTime::fromMSecsSinceStartOfDay(m_mediaPlayer->position()).toString("hh:mm:ss"))
                           .arg(QTime::fromMSecsSinceStartOfDay(m_mediaPlayer->duration()).toString("hh:mm:ss")));
}

void MainWindow::saveLastFrame(const QVideoFrame &frame)
{
    m_lastFrame = frame;
}

void MainWindow::saveCurrentFrame()
{
    QVideoFrame frame = m_lastFrame;
    QVideoFrame::PixelFormat pixelFormat = m_lastFrame.pixelFormat();

    if (pixelFormat !=QVideoFrame::Format_YUV420P)
        return;

    int width = frame.width();
    int height = frame.height();

    m_lastFrame.map(QAbstractVideoBuffer::ReadOnly);

    unsigned char *yBits = new unsigned char [width * height];
    memcpy(yBits, const_cast<uchar *>(frame.bits(0)), width * height);

    int uvSize = width * height / 4;

    unsigned char *uBits = new unsigned char [uvSize];
    memcpy(uBits, frame.bits(1), uvSize);

    unsigned char *vBits = new unsigned char [uvSize];
    memcpy(vBits, frame.bits(2), uvSize);

    frame.unmap();

    unsigned char *rgbImage = new unsigned char[width * height * 3];

    int y;
    int u;
    int v;

    int y1;
    int v1;
    int u1;
    int uv1;

    int r;
    int g;
    int b;

    int row;
    int uvIndex;


    for (int i = 0; i < width * height; ++i) {
        y = yBits[i];
        y1 = y << 12;

        row = i / width / 2;
        uvIndex = (width / 2) * row + (i % width) / 2;
        u = uBits[uvIndex];
        v = vBits[uvIndex];

        u = u - 128;
        v = v - 128;

        v1  = (5727 * v);
        uv1 = -(1617 * u) - (2378 * v);
        u1  = (8324 * u);

        r = (y1 + v1) >> 12;
        g = (y1 + uv1) >> 12;
        b = (y1 + u1) >> 12;

        if (r < 0)
            r = 0;
        else if (r > 255)
            r = 255;

        if (g < 0)
            g = 0;
        else if (g > 255)
            g = 255;

        if (b < 0)
            b = 0;
        else if (b > 255)
            b = 255;

        rgbImage[i * 3] = (unsigned char)r;
        rgbImage[i * 3 + 1] = (unsigned char)g;
        rgbImage[i * 3 + 2] = (unsigned char)b;
    }

    QImage image(rgbImage,
                 width,
                 height,
                 QImage::Format_RGB888);

    delete [] yBits;
    delete [] uBits;
    delete [] vBits;

    m_mediaPlayer->pause();

    if (image.isNull()) {
        delete [] rgbImage;
        return;
    }
    m_mediaPlayer->pause();
    QSettings settings;
    settings.beginGroup(SETTINGS_MAIN_WINDOW_GROUP);
    QString oldDir = settings.value(SETTINGS_MAIN_WINDOW_LAST_SAVE_DIR).toString();

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save to image"), oldDir, tr("PNG Image (*.png)"));
    if (fileName.isEmpty())
        return;

    QFileInfo fileInfo (fileName);
    settings.setValue(SETTINGS_MAIN_WINDOW_LAST_SAVE_DIR, fileInfo.path());
    settings.endGroup();

    if (!image.save(fileName))
        QMessageBox::warning(this, tr("Save error"), tr("Error on save to \"%0\".")
                             .arg(fileInfo.filePath()));

    delete [] rgbImage;
}

void MainWindow::playFile(const QUrl &fileName) const
{
    if (m_mediaPlayer->currentMedia().canonicalUrl() == fileName) {
        m_mediaPlayer->play();
        return;
    }
    m_mediaPlaylist->clear();
    m_mediaPlaylist->addMedia(fileName);
    m_mediaPlaylist->setCurrentIndex(0);
    m_mediaPlayer->play();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}
