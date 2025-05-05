#include "tetriswidget.h"
#include <cstring>
#include <QFile>
#include <QHBoxLayout>

const QVector<QRgb> TetrisWidget::colors = {
    0x0000FF, 0xFF0000, 0x00FF00, 0xFF00FF,
    0xFFFF00, 0x00FFFF, 0xFFA500
};

// 构造函数
TetrisWidget::TetrisWidget(QWidget *parent) : QWidget(parent),
    currentX(0), currentY(0), score(0), isPaused(false) {
    
    setFixedSize(BoardWidth * 30 + 150, VisibleBoardHeight * 30);
    initUI();

    bgmPlayer.setSource(QUrl("qrc:/sounds/bgm.wav"));
    bgmPlayer.setLoopCount(QSoundEffect::Infinite);
    bgmPlayer.setVolume(0.5f);
    bgmPlayer.play();

    clearSound.setSource(QUrl("qrc:/sounds/clear.wav"));
    clearSound.setVolume(0.8f);

    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &TetrisWidget::updateTimeDisplay);
    gameTimer->start(1000);
    totalTime.start();

    clearBoard();
    newPiece();
    timer.start(500, this);
}

// 析构函数
TetrisWidget::~TetrisWidget() {
    bgmPlayer.stop();
}

// 初始化界面
void TetrisWidget::initUI() {
    QWidget *infoPanel = new QWidget(this);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoPanel);
    
    scoreLabel = new QLabel("得分：0", infoPanel);
    scoreLabel->setStyleSheet("font: bold 18px; color: #FFD700;");
    infoLayout->addWidget(scoreLabel);

    timeLabel = new QLabel("时间：00:00", infoPanel);
    timeLabel->setStyleSheet("font: bold 18px; color: #00FF00;");
    infoLayout->addWidget(timeLabel);

    infoLayout->addStretch();
    infoPanel->setGeometry(BoardWidth * 30 + 10, 10, 120, 200);
}

// 更新时间显示
void TetrisWidget::updateTimeDisplay() {
    int seconds = totalTime.elapsed() / 1000;
    timeLabel->setText(
        QString("时间：%1:%2")
        .arg(seconds / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'))
    );
    scoreLabel->setText(QString("得分：%1").arg(score));
}

// ---------- 核心游戏逻辑实现 ----------
void TetrisWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    drawBoard(p);
    drawPiece(p, currentPiece, currentX, currentY);
}

// 键盘事件处理
void TetrisWidget::keyPressEvent(QKeyEvent *event) {
    if (isPaused && event->key() != Qt::Key_P) return;

    switch (event->key()) {
    case Qt::Key_P:
        isPaused = !isPaused;
        if (isPaused) {
            timer.stop();
            gameTimer->stop();
            bgmPlayer.stop();
            QMessageBox::information(this, "游戏暂停", "按 P 键继续游戏");
        } else {
            timer.start(500, this);
            gameTimer->start();
            bgmPlayer.play();
        }
        break;
    case Qt::Key_Left:
        tryMove(currentPiece, currentX - 1, currentY);
        break;
    case Qt::Key_Right:
        tryMove(currentPiece, currentX + 1, currentY);
        break;
    case Qt::Key_Down:
        tryMove(currentPiece.rotatedRight(), currentX, currentY);
        break;
    case Qt::Key_Up:
        tryMove(currentPiece.rotatedLeft(), currentX, currentY);
        break;
    case Qt::Key_Space:
        dropDown();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

// 定时器事件
void TetrisWidget::timerEvent(QTimerEvent *event) {
    if (isPaused) return;

    if (event->timerId() == timer.timerId()) {
        oneLineDown();
    } else {
        QWidget::timerEvent(event);
    }
}

// 清空棋盘
void TetrisWidget::clearBoard() {
    for (int i = 0; i < BoardHeight; ++i)
        for (int j = 0; j < BoardWidth; ++j)
            board[i][j] = 0;
}

// 方块旋转方法
TetrisWidget::Piece TetrisWidget::Piece::rotatedRight() const {
    Piece result;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            result.shape[i][j] = shape[3 - j][i];
    result.color = color;
    return result;
}

TetrisWidget::Piece TetrisWidget::Piece::rotatedLeft() const {
    Piece result;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            result.shape[i][j] = shape[j][3 - i];
    result.color = color;
    return result;
}

// 生成新方块
void TetrisWidget::newPiece() {
    static const int shapes[7][4][4] = {
        {{1,1,1,1}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{1,1,1,0}, {1,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{1,1,1,0}, {0,0,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{1,1,1,0}, {0,1,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}
    };

    int index = QRandomGenerator::global()->bounded(7);
    currentPiece.color = colors[index];
    memcpy(currentPiece.shape, shapes[index], sizeof(currentPiece.shape));
    
    currentX = BoardWidth / 2 - 2;
    currentY = 0;

    if (!tryMove(currentPiece, currentX, currentY)) {
        timer.stop();
        QMessageBox::information(this, "Game Over", "Game Over!");
    }
}

// 移动检测
bool TetrisWidget::tryMove(const Piece &newPiece, int newX, int newY) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (newPiece.shape[i][j]) {
                int x = newX + j;
                int y = newY + i;
                if (x < 0 || x >= BoardWidth || y >= VisibleBoardHeight)
                    return false;
                if (y >= 0 && board[y][x])
                    return false;
            }
        }
    }
    currentPiece = newPiece;
    currentX = newX;
    currentY = newY;
    update();
    return true;
}

// 快速下落
void TetrisWidget::dropDown() {
    while (oneLineDown());
}

// 单步下落
bool TetrisWidget::oneLineDown() {
    if (!tryMove(currentPiece, currentX, currentY + 1)) {
        pieceDropped();
        return false;
    }
    return true;
}

// 方块落地处理
void TetrisWidget::pieceDropped() {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (currentPiece.shape[i][j]) {
                int x = currentX + j;
                int y = currentY + i;
                if (y >= 0) board[y][x] = currentPiece.color;
            }
        }
    }
    removeFullLines();
    newPiece();
}

// 消除满行
void TetrisWidget::removeFullLines() {
    int numFullLines = 0;
    for (int y = BoardHeight - 1; y >= 0; --y) {
        bool full = true;
        for (int x = 0; x < BoardWidth; ++x) {
            if (!board[y][x]) {
                full = false;
                break;
            }
        }
        if (full) {
            ++numFullLines;
            for (int yy = y; yy > 0; --yy)
                memcpy(board[yy], board[yy - 1], BoardWidth * sizeof(int));
            ++y;
        }
    }
    if (numFullLines > 0) {
        score += 10 * numFullLines;
        emit scoreChanged(score);
        clearSound.play();
        update();
    }
}

// 绘制棋盘
void TetrisWidget::drawBoard(QPainter &p) {
    for (int y = 0; y < VisibleBoardHeight; ++y) {
        for (int x = 0; x < BoardWidth; ++x) {
            if (board[y][x]) {
                p.fillRect(x * 30, y * 30, 29, 29, QColor(board[y][x]));
            }
        }
    }
}

// 绘制当前方块
void TetrisWidget::drawPiece(QPainter &p, const Piece &piece, int x, int y) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (piece.shape[i][j]) {
                p.fillRect((x + j) * 30, (y + i) * 30, 29, 29, QColor(piece.color));
            }
        }
    }
}
