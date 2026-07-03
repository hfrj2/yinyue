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
#include <QVBoxLayout>
#include <QTextStream>
#include <QRegularExpression>
#include <QPropertyAnimation>
#include <algorithm>

// ==================== 构造 & 析构 ====================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_player(nullptr)
    , audioOutput(nullptr)
    , m_currentIndex(-1)
    , m_lyricTimer(nullptr)
    , m_currentLyricIndex(-1)
    , m_lyricLabel(nullptr)
    , m_lyricOpacityEffect(nullptr)
{
    // 初始化歌词标签数组
    for (int i = 0; i < LYRICS_LINE_COUNT; ++i) {
        m_lyricLines[i] = nullptr;
    }

#ifdef Q_OS_WIN
    qputenv("QT_MEDIA_BACKEND", "windows");
#endif

    ui->setupUi(this);

    setWindowTitle("朝琴音乐播放器");
    setFixedSize(1020, 720);
    this->setWindowIcon(QIcon(":/icon/app.ico"));

    initUI();
    initMediaPlayer();
    initMusicList();
    initButton();
    initConnections();

    // 设置歌词显示
    setupLyricWidget();

    // 加载音乐列表
    loadMusicList("D:\\App\\qt_project\\yingyue\\musicSet");

    // 自动播放第一首
    if (!m_musicList.isEmpty()) {
        playMusic(0);
    }
}

MainWindow::~MainWindow()
{
    if (m_lyricTimer) {
        m_lyricTimer->stop();
    }
    delete ui;
}

// ==================== UI 初始化 ====================

void MainWindow::initUI()
{
    setBackGround(":/image/12.png");
    setupListStyle();
}

void MainWindow::initMediaPlayer()
{
    m_player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(audioOutput);
    audioOutput->setVolume(0.8f);
}

void MainWindow::initMusicList()
{
    // 设置选择模式
    ui->music_list->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MainWindow::initConnections()
{
    // 按钮连接
    connect(ui->playBtn, &QPushButton::clicked, this, &MainWindow::handlePlaySlot);
    connect(ui->prevBtn, &QPushButton::clicked, this, &MainWindow::handlePrevSlot);
    connect(ui->nextBtn, &QPushButton::clicked, this, &MainWindow::handleNextSlot);

    // 列表点击/双击
    connect(ui->music_list, &QListWidget::itemClicked, this, &MainWindow::onMusicItemClicked);
    connect(ui->music_list, &QListWidget::itemDoubleClicked, this, &MainWindow::onMusicItemDoubleClicked);

    // 媒体加载完成后自动播放
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia) {
            m_player->play();
            ui->playBtn->setIcon(QIcon(":/icon/pause.png"));
            if (m_lyricTimer) {
                m_lyricTimer->start();
            }
        }
    });

    // 播放状态变化时更新按钮图标
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::PlayingState) {
            ui->playBtn->setIcon(QIcon(":/icon/pause.png"));
            if (m_lyricTimer) {
                m_lyricTimer->start();
            }
        } else {
            ui->playBtn->setIcon(QIcon(":/icon/play.png"));
            if (state == QMediaPlayer::StoppedState) {
                if (m_lyricTimer) {
                    m_lyricTimer->stop();
                }
            }
        }
    });

    // 播放结束后自动播下一首
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            handleNextSlot();
        }
    });

    // 播放错误处理
    connect(m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error) {
        QMessageBox::warning(this, "播放错误",
                             QString("播放出错: %1").arg(m_player->errorString()));
    });

    // 歌词定时器
    m_lyricTimer = new QTimer(this);
    m_lyricTimer->setInterval(100);
    connect(m_lyricTimer, &QTimer::timeout, this, &MainWindow::updateLyricMultiLine);
    // 不立即启动，等待播放时启动
}

void MainWindow::setupListStyle()
{
    ui->music_list->setStyleSheet(
        // ===== 整体列表样式 =====
        "QListWidget {"
        "    background-color: #1a0a2e;"
        "    color: #e8d5f5;"
        "    font-size: 14px;"
        "    font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif;"
        "    border: 2px solid #4a2a6e;"
        "    border-radius: 12px;"
        "    padding: 8px;"
        "    outline: none;"
        "}"
        ""
        "QListWidget::item {"
        "    padding: 8px 12px;"
        "    border-radius: 6px;"
        "    margin: 2px 0px;"
        "}"
        ""
        "QListWidget::item:selected {"
        "    background-color: #b39ddb;"
        "    color: #1a0a2e;"
        "    border: none;"
        "    border-radius: 6px;"
        "}"
        ""
        "QListWidget::item:selected:focus {"
        "    background-color: #ce93d8;"
        "    color: #1a0a2e;"
        "    border-radius: 6px;"
        "}"
        ""
        "QListWidget::item:hover {"
        "    background-color: #3a2a5e;"
        "    color: #f0e5f5;"
        "    border-radius: 6px;"
        "}"
        ""
        "QScrollBar:vertical {"
        "    background: #1a0a2e;"
        "    width: 10px;"
        "    border-radius: 5px;"
        "    margin: 2px 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: #7e57c2;"
        "    border-radius: 5px;"
        "    min-height: 30px;"
        "    margin: 2px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: #b39ddb;"
        "}"
        "QScrollBar::add-line:vertical,"
        "QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "    background: transparent;"
        "}"
        "QScrollBar::add-page:vertical,"
        "QScrollBar::sub-page:vertical {"
        "    background: transparent;"
        "}"
        ""
        "QScrollBar:horizontal {"
        "    background: #1a0a2e;"
        "    height: 10px;"
        "    border-radius: 5px;"
        "    margin: 0px 2px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background: #7e57c2;"
        "    border-radius: 5px;"
        "    min-width: 30px;"
        "    margin: 2px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "    background: #b39ddb;"
        "}"
        "QScrollBar::add-line:horizontal,"
        "QScrollBar::sub-line:horizontal {"
        "    width: 0px;"
        "    background: transparent;"
        "}"
        "QScrollBar::add-page:horizontal,"
        "QScrollBar::sub-page:horizontal {"
        "    background: transparent;"
        "}"
        );
}

// ==================== 背景 & 按钮样式 ====================

void MainWindow::setBackGround(const QString &filename)
{
    QPixmap pixmap(filename);
    QSize windowSize = this->size();
    QPixmap scalPixmap = pixmap.scaled(windowSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, QBrush(scalPixmap));
    this->setPalette(palette);
}

void MainWindow::setButtonStyle(QPushButton *button, const QString &filename)
{
    button->setFixedSize(40, 40);
    button->setIcon(QIcon(filename));
    button->setIconSize(QSize(40, 40));
    button->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(255, 255, 255, 30);"
        "    border-radius: 20px;"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(255, 255, 255, 50);"
        "}"
        );
}

void MainWindow::initButton()
{
    setButtonStyle(ui->prevBtn, ":/icon/prev.png");
    setButtonStyle(ui->playBtn, ":/icon/play.png");
    setButtonStyle(ui->nextBtn, ":/icon/next.png");
    setButtonStyle(ui->modeBtn, ":/icon/mode.png");
    setButtonStyle(ui->listBtn, ":/icon/list.png");
}

// ==================== 音乐列表加载 ====================

void MainWindow::loadMusicList(const QString &dirPath)
{
    QDir dir(dirPath);

    if (!dir.exists()) {
        qDebug() << "音乐文件夹不存在：" << dirPath;
        QMessageBox::warning(this, "文件夹", "音乐文件夹打开失败: " + dirPath);
        return;
    }

    // 清空列表
    ui->music_list->clear();
    m_musicList.clear();

    // 支持的音频格式
    QStringList filters;
    filters << "*.mp3" << "*.wav" << "*.flac" << "*.m4a" << "*.aac" << "*.ogg";

    // 获取所有音乐文件
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo &fileInfo : fileList) {
        m_musicList.append(fileInfo.absoluteFilePath());
        ui->music_list->addItem(fileInfo.baseName());
        qDebug() << "添加音乐：" << fileInfo.fileName();
    }

    qDebug() << "共加载" << m_musicList.size() << "首音乐";
}

// ==================== 播放控制 ====================

void MainWindow::playMusic(int index)
{
    if (index < 0 || index >= m_musicList.size()) {
        qDebug() << "无效的索引：" << index;
        return;
    }

    m_currentIndex = index;
    QString filePath = m_musicList[index];

    m_player->setSource(QUrl::fromLocalFile(filePath));
    ui->music_list->setCurrentRow(index);
    ui->music_list->scrollToItem(ui->music_list->currentItem(),
                                 QAbstractItemView::PositionAtCenter);

    QFileInfo fileInfo(filePath);
    setWindowTitle("朝琴音乐播放器 - " + fileInfo.fileName());

    // 加载歌词
    loadLyric(filePath);
    m_currentLyricIndex = -1;

    // 重置多行歌词显示
    for (int i = 0; i < LYRICS_LINE_COUNT; ++i) {
        if (m_lyricLines[i]) {
            if (i == 4) {  // 中间行
                m_lyricLines[i]->setText("加载歌词...");
            } else {
                m_lyricLines[i]->setText("");
            }
        }
    }

    qDebug() << "播放：" << fileInfo.fileName();
}

void MainWindow::handlePlaySlot()
{
    if (m_musicList.isEmpty()) {
        QMessageBox::information(this, "提示", "播放列表为空，请添加音乐文件");
        return;
    }

    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        if (m_lyricTimer) {
            m_lyricTimer->stop();
        }
    } else {
        if (m_currentIndex < 0) {
            playMusic(0);
        } else {
            m_player->play();
            if (m_lyricTimer) {
                m_lyricTimer->start();
            }
        }
    }
}

void MainWindow::handlePrevSlot()
{
    if (m_musicList.isEmpty()) return;

    int newIndex = m_currentIndex - 1;
    if (newIndex < 0) {
        newIndex = m_musicList.size() - 1;
    }
    playMusic(newIndex);
}

void MainWindow::handleNextSlot()
{
    if (m_musicList.isEmpty()) return;

    int newIndex = m_currentIndex + 1;
    if (newIndex >= m_musicList.size()) {
        newIndex = 0;
    }
    playMusic(newIndex);
}

void MainWindow::onMusicItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    int row = ui->music_list->row(item);
    if (row >= 0 && row < m_musicList.size()) {
        QString filePath = m_musicList[row];
        m_player->setSource(QUrl::fromLocalFile(filePath));
        m_currentIndex = row;

        if (m_player->playbackState() != QMediaPlayer::PlayingState) {
            m_player->play();
            if (m_lyricTimer) {
                m_lyricTimer->start();
            }
        }

        QFileInfo fileInfo(filePath);
        setWindowTitle("朝琴音乐播放器 - " + fileInfo.fileName());

        // 加载歌词
        loadLyric(filePath);
        m_currentLyricIndex = -1;

        // 重置歌词显示
        for (int i = 0; i < LYRICS_LINE_COUNT; ++i) {
            if (m_lyricLines[i]) {
                if (i == 4) {
                    m_lyricLines[i]->setText("加载歌词...");
                } else {
                    m_lyricLines[i]->setText("");
                }
            }
        }
    }
}

void MainWindow::onMusicItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;

    int row = ui->music_list->row(item);
    if (row >= 0 && row < m_musicList.size()) {
        playMusic(row);
    }
}

void MainWindow::updateCurrentSongDisplay()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_musicList.size()) {
        QFileInfo fileInfo(m_musicList[m_currentIndex]);
        setWindowTitle("朝琴音乐播放器 - " + fileInfo.fileName());
    }
}

// ==================== 歌词显示设置（9行，当前行居中） ====================

void MainWindow::setupLyricWidget()
{
    // 创建歌词容器
    QWidget *lyricContainer = new QWidget(this);
    lyricContainer->setObjectName("lyricContainer");
    lyricContainer->setFixedSize(520, 420);
    lyricContainer->setStyleSheet(
        "QWidget#lyricContainer {"
        "    background-color: rgba(179, 157, 219, 200);"
        "    border-radius: 12px;"
        "    border: 2px solid #7e57c2;"
        "}"
        );

    // 垂直布局
    QVBoxLayout *layout = new QVBoxLayout(lyricContainer);
    layout->setContentsMargins(20, 15, 20, 15);
    layout->setSpacing(2);

    // 创建9个歌词标签（当前行在索引4，居中）
    for (int i = 0; i < LYRICS_LINE_COUNT; ++i) {
        m_lyricLines[i] = new QLabel(lyricContainer);
        m_lyricLines[i]->setAlignment(Qt::AlignCenter);
        m_lyricLines[i]->setWordWrap(true);

        if (i == 4) {  // 中间行 - 当前歌词（高亮）
            m_lyricLines[i]->setStyleSheet(
                "QLabel {"
                "    color: #ffffff;"
                "    font-size: 24px;"
                "    font-weight: bold;"
                "    font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif;"
                "    background-color: transparent;"
                "    padding: 5px;"
                "}"
                );
            m_lyricLines[i]->setText("等待播放...");
        } else {
            // 其他行 - 根据距离调整透明度和字体大小
            int distance = qAbs(i - 4);
            int opacity = 255 - distance * 40;
            if (opacity < 40) opacity = 40;

            int fontSize = 22 - distance * 3;
            if (fontSize < 12) fontSize = 12;

            m_lyricLines[i]->setStyleSheet(
                QString("QLabel {"
                        "    color: rgba(255, 255, 255, %1);"
                        "    font-size: %2px;"
                        "    font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif;"
                        "    background-color: transparent;"
                        "    padding: 2px;"
                        "}")
                    .arg(opacity)
                    .arg(fontSize)
                );
            m_lyricLines[i]->setText("");
        }

        layout->addWidget(m_lyricLines[i]);
    }

    lyricContainer->setLayout(layout);

    // 将容器添加到UI中的lyricWidget
    if (ui->lyricWidget) {
        QVBoxLayout *containerLayout = new QVBoxLayout(ui->lyricWidget);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->addWidget(lyricContainer);
        ui->lyricWidget->setLayout(containerLayout);
    } else {
        lyricContainer->move(50, 80);
        lyricContainer->show();
    }

    // 保存主标签引用（中间行）
    m_lyricLabel = m_lyricLines[4];

    // 设置透明度效果（用于淡入淡出动画）
    m_lyricOpacityEffect = new QGraphicsOpacityEffect(m_lyricLabel);
    m_lyricOpacityEffect->setOpacity(1.0);
    m_lyricLabel->setGraphicsEffect(m_lyricOpacityEffect);
}

// ==================== 更新歌词行 ====================

void MainWindow::updateLyricLines(int currentIndex)
{
    if (m_lyricList.isEmpty()) {
        for (int i = 0; i < LYRICS_LINE_COUNT; ++i) {
            if (m_lyricLines[i]) {
                if (i == 4) {
                    m_lyricLines[i]->setText("暂无歌词");
                } else {
                    m_lyricLines[i]->setText("");
                }
            }
        }
        return;
    }

    // 计算显示的歌词范围：当前行在中间（索引4）
    int startIndex = currentIndex - 4;

    // 填充9行歌词
    for (int i = 0; i < LYRICS_LINE_COUNT; ++i) {
        if (!m_lyricLines[i]) continue;

        int lyricIndex = startIndex + i;
        if (lyricIndex >= 0 && lyricIndex < m_lyricList.size()) {
            m_lyricLines[i]->setText(m_lyricList[lyricIndex].text);
        } else {
            m_lyricLines[i]->setText("");
        }
    }
}

// ==================== 多行歌词更新 ====================

void MainWindow::updateLyricMultiLine()
{
    if (!m_player || m_player->playbackState() != QMediaPlayer::PlayingState) {
        return;
    }

    if (m_lyricList.isEmpty()) {
        for (int i = 0; i < LYRICS_LINE_COUNT; ++i) {
            if (m_lyricLines[i]) {
                if (i == 4) {
                    m_lyricLines[i]->setText("暂无歌词");
                } else {
                    m_lyricLines[i]->setText("");
                }
            }
        }
        return;
    }

    qint64 position = m_player->position();
    int currentIndex = findLyricIndex(static_cast<int>(position));

    if (currentIndex != m_currentLyricIndex && currentIndex >= 0) {
        m_currentLyricIndex = currentIndex;

        // 更新当前行（带动画）
        QString currentText = m_lyricList[currentIndex].text;
        updateLyricWithAnimation(currentText);

        // 更新所有歌词行
        updateLyricLines(currentIndex);
    }
}

// ==================== 带淡入淡出动画的歌词更新 ====================

void MainWindow::updateLyricWithAnimation(const QString &text)
{
    if (!m_lyricLabel || !m_lyricOpacityEffect) return;

    // 创建淡出动画（旧歌词消失）
    QPropertyAnimation *fadeOut = new QPropertyAnimation(m_lyricOpacityEffect, "opacity");
    fadeOut->setDuration(150);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);

    // 创建淡入动画（新歌词出现）
    QPropertyAnimation *fadeIn = new QPropertyAnimation(m_lyricOpacityEffect, "opacity");
    fadeIn->setDuration(150);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);

    // 连接动画：淡出完成后更新文字，然后淡入
    connect(fadeOut, &QPropertyAnimation::finished, this, [this, text, fadeIn]() {
        m_lyricLabel->setText(text);
        fadeIn->start();
    });

    // 动画结束后自动删除
    connect(fadeIn, &QPropertyAnimation::finished, fadeIn, &QPropertyAnimation::deleteLater);
    connect(fadeOut, &QPropertyAnimation::finished, fadeOut, &QPropertyAnimation::deleteLater);

    // 开始淡出动画
    fadeOut->start();
}

// ==================== 歌词文件加载 & 解析 ====================

void MainWindow::loadLyric(const QString &musicPath)
{
    m_lyricList.clear();
    m_currentLyricIndex = -1;

    // 将 .mp3 替换为 .lrc
    QString lyricPath = musicPath;
    lyricPath.replace(QRegularExpression("\\.(mp3|wav|flac|m4a|aac|ogg)$",
                                         QRegularExpression::CaseInsensitiveOption),
                      ".lrc");

    QFile file(lyricPath);
    if (!file.exists()) {
        qDebug() << "歌词文件不存在：" << lyricPath;
        return;
    }

    parseLyric(lyricPath);
    qDebug() << "加载歌词：" << lyricPath << "，共" << m_lyricList.size() << "行";
}

void MainWindow::parseLyric(const QString &lyricPath)
{
    QFile file(lyricPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开歌词文件：" << lyricPath;
        return;
    }

    QTextStream in(&file);
#ifdef Q_OS_WIN
    in.setEncoding(QStringConverter::Utf8);
#else
    in.setCodec("UTF-8");
#endif

    // 正则匹配 [mm:ss.xx] 或 [mm:ss]
    QRegularExpression regex("\\[(\\d+):(\\d+)(?:\\.(\\d+))?\\](.*)");

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QRegularExpressionMatch match = regex.match(line);

        if (match.hasMatch()) {
            int minutes = match.captured(1).toInt();
            int seconds = match.captured(2).toInt();
            QString msStr = match.captured(3);
            int milliseconds = msStr.isEmpty() ? 0 : msStr.toInt() * 10;
            QString text = match.captured(4).trimmed();

            int totalMs = (minutes * 60 + seconds) * 1000 + milliseconds;

            LyricLine lyricLine;
            lyricLine.time = totalMs;
            lyricLine.text = text.isEmpty() ? "..." : text;
            m_lyricList.append(lyricLine);
        }
    }

    file.close();

    // 按时间排序
    std::sort(m_lyricList.begin(), m_lyricList.end(),
              [](const LyricLine &a, const LyricLine &b) {
                  return a.time < b.time;
              });
}

int MainWindow::findLyricIndex(int position)
{
    if (m_lyricList.isEmpty()) {
        return -1;
    }

    // 二分查找：找到最后一个 time <= position 的歌词
    int left = 0;
    int right = m_lyricList.size() - 1;
    int result = -1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (m_lyricList[mid].time <= position) {
            result = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return result;
}