#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QPushButton>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QStringList>
#include <QListWidgetItem>
#include <QVector>
#include <QTimer>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// 歌词结构体
struct LyricLine {
    int time;           // 毫秒
    QString text;       // 歌词文本
};

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
    void onMusicItemClicked(QListWidgetItem *item);
    void onMusicItemDoubleClicked(QListWidgetItem *item);
    void updateLyricMultiLine();     // 多行歌词更新

private:
    void initUI();
    void initMediaPlayer();
    void initMusicList();
    void initConnections();
    void loadMusicList(const QString &dirPath);
    void playMusic(int index);
    void updateCurrentSongDisplay();
    void setupLyricWidget();
    void loadLyric(const QString &musicPath);
    void parseLyric(const QString &lyricPath);
    int findLyricIndex(int position);
    void updateLyricWithAnimation(const QString &text);
    void updateLyricLines(int currentIndex);  // 更新所有歌词行

    void setBackGround(const QString &filename);
    void setButtonStyle(QPushButton *button, const QString &filename);
    void initButton();
    void setupListStyle();

private:
    Ui::MainWindow *ui;
    QMediaPlayer *m_player;
    QAudioOutput *audioOutput;
    QStringList m_musicList;
    int m_currentIndex;

    // 歌词相关
    QVector<LyricLine> m_lyricList;
    QTimer *m_lyricTimer;
    int m_currentLyricIndex;
    QLabel *m_lyricLabel;

    // 多行歌词相关 - 9行（当前行居中）
    static constexpr int LYRICS_LINE_COUNT = 9;
    QLabel *m_lyricLines[LYRICS_LINE_COUNT];

    // 歌词动画
    QGraphicsOpacityEffect *m_lyricOpacityEffect;
};

#endif // MAINWINDOW_H