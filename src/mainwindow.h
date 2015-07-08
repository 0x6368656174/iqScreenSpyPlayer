#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QVideoProbe>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    virtual void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    void initSettings() const;
    void loadSettings();
    void saveSettings() const;

    void changeIntefaceOnMediaStateChanged() const;
    void playPause() const;
    void changeTimeLabelText() const;

    void saveLastFrame(const QVideoFrame &frame);
    void saveCurrentFrame();

    void playFile(const QUrl &fileName) const;

private:
    Ui::MainWindow *ui;
    QMediaPlayer *m_mediaPlayer;
    QMediaPlaylist *m_mediaPlaylist;
    QVideoProbe *m_videoProbe;
    QVideoFrame m_lastFrame;
};

#endif // MAINWINDOW_H
