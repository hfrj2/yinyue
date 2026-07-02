#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QIcon>
#include <QPixmap>
#include <QPalette>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include <QFile>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_currentIndex(-1) {

#ifdef Q_OS_WIN
    qputenv("QT_MEDIA_BACKEND", "windows");
#endif

    ui->setupUi(this);

    setWindowTitle("朝琴音乐播放器");

    // 创建播放器
    m_player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(audioOutput);

    // 设置背景
    setBackGround(":/image/12.png");
    setFixedSize(1020, 720);

    // 初始化按钮
    initButton();

    // 加载音乐列表
    loadMusicList();

    audioOutput->setVolume(0.8f);

    // 媒体加载完成后自动播放
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia) {
            m_player->play();
            ui->playBtn->setIcon(QIcon(":/icon/pause.png"));
        }
    });

    // 播放状态变化时更新按钮图标
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::PlayingState) {
            ui->playBtn->setIcon(QIcon(":/icon/pause.png"));
        } else {
            ui->playBtn->setIcon(QIcon(":/icon/play.png"));
        }
    });

    // ★★★ 播放结束后自动播下一首 ★★★
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            handleNextSlot();
        }
    });

    // 自动播放第一首
    if (!m_musicList.isEmpty()) {
        playMusic(0);
    }
}

void MainWindow::loadMusicList() {
    // 音乐文件夹路径
    QString musicDir = "D:\\App\\qt_project\\yingyue\\musicSet";
    QDir dir(musicDir);

    if (!dir.exists()) {
        qDebug() << "音乐文件夹不存在：" << musicDir;
        return;
    }

    // 支持的音频格式
    QStringList filters;
    filters << "*.mp3" << "*.wav" << "*.flac" << "*.m4a" << "*.aac";

    // 获取所有音乐文件
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo &fileInfo : fileList) {
        m_musicList.append(fileInfo.absoluteFilePath());
        qDebug() << "添加音乐：" << fileInfo.fileName();
    }

    qDebug() << "共加载" << m_musicList.size() << "首音乐";
//新的
    loadAppointMusicDir(musicDir);
}

void MainWindow::playMusic(int index) {
    if (index < 0 || index >= m_musicList.size()) return;

    m_currentIndex = index;
    QString filePath = m_musicList[index];

    // 设置音源
    m_player->setSource(QUrl::fromLocalFile(filePath));

    // 更新窗口标题
    QFileInfo fileInfo(filePath);
    setWindowTitle("朝琴音乐播放器 - " + fileInfo.fileName());

    qDebug() << "播放：" << fileInfo.fileName();
}

void MainWindow::setButtonStyle(QPushButton *button, const QString &filename) {
    button->setFixedSize(40, 40);
    button->setIcon(QIcon(filename));
    button->setIconSize(QSize(ui->prevBtn->width(), ui->prevBtn->height()));
    button->setStyleSheet("background-color:transparent");
}

void MainWindow::initButton() {
    setButtonStyle(ui->prevBtn, ":/icon/prev.png");
    setButtonStyle(ui->playBtn, ":/icon/play.png");
    setButtonStyle(ui->nextBtn, ":/icon/next.png");
    setButtonStyle(ui->modeBtn, ":/icon/mode.png");
    setButtonStyle(ui->listBtn, ":/icon/list.png");

    connect(ui->playBtn, &QPushButton::clicked, this, &MainWindow::handlePlaySlot);
    connect(ui->prevBtn, &QPushButton::clicked, this, &MainWindow::handlePrevSlot);
    connect(ui->nextBtn, &QPushButton::clicked, this, &MainWindow::handleNextSlot);
}

// 播放/暂停
void MainWindow::handlePlaySlot() {
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        m_player->play();
    }
}

// 上一曲
void MainWindow::handlePrevSlot() {
    if (m_musicList.isEmpty()) return;

    int newIndex = m_currentIndex - 1;
    if (newIndex < 0) {
        newIndex = m_musicList.size() - 1;  // 循环到最后一首
    }
    playMusic(newIndex);
}

// 下一曲
void MainWindow::handleNextSlot() {
    if (m_musicList.isEmpty()) return;

    int newIndex = m_currentIndex + 1;
    if (newIndex >= m_musicList.size()) {
        newIndex = 0;  // 循环到第一首
    }
    playMusic(newIndex);
}

void MainWindow::setBackGround(const QString &filename) {
    QPixmap pixmap(filename);
    QSize windowSize = this->size();
    QPixmap scalPixmap = pixmap.scaled(windowSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, QBrush(scalPixmap));
    this->setPalette(palette);
}

void MainWindow::loadAppointMusicDir(const QString & filepath)
{
    QDir dir(filepath);
    if(dir.exists()==false)
    {
        QMessageBox::warning(this,"文件夹","文件夹打开失败");
        return;
    }

    QFileInfoList fileList = dir.entryInfoList(QDir::Files);

    for(auto element : fileList)
{
        if(element.suffix()=="mp3")
        {
            ui->music_list->addItem(element.baseName());
        }
    }
}

MainWindow::~MainWindow() {
    delete ui;
}