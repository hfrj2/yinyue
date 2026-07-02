#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QPushButton>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void handlePlaySlot();
    void handlePrevSlot();
    void handleNextSlot();

private:
    void setBackGround(const QString & filename);
    void setButtonStyle(QPushButton * button,const QString & filename);
    void initButton();
    void loadMusicList();
    void playMusic(int index);
    void loadAppointMusicDir(const QString & filepath);

private:
    Ui::MainWindow *ui;
    QMediaPlayer *m_player;
    QAudioOutput *audioOutput;
    QStringList m_musicList;    // 音乐文件路径列表
    int m_currentIndex;         // 当前播放索引
};

#endif // MAINWINDOW_H