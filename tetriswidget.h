#ifndef TETRISWIDGET_H
#define TETRISWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QKeyEvent>
#include <QTimer>
#include <QVector>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QSoundEffect>
#include <QLabel>
#include <QVBoxLayout>
#include <QElapsedTimer>

class TetrisWidget : public QWidget {
    Q_OBJECT
public:
    explicit TetrisWidget(QWidget *parent = nullptr);
    ~TetrisWidget();
    void paintEvent(QPaintEvent *event) override;

signals:
    void scoreChanged(int score);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private slots:
    void updateTimeDisplay();

private:
    struct Piece {
        int shape[4][4];
        QRgb color;
        
        Piece rotatedRight() const;
        Piece rotatedLeft() const;
    };

    static const int BoardWidth = 10;
    static const int VisibleBoardHeight = 20;
    static const int BoardHeight = 22;
    static const QVector<QRgb> colors;

    int board[BoardHeight][BoardWidth];
    Piece currentPiece;
    int currentX;
    int currentY;
    QBasicTimer timer;
    int score;
    bool isPaused;

    QLabel *timeLabel;
    QLabel *scoreLabel;
    QTimer *gameTimer;
    QElapsedTimer totalTime;

    QSoundEffect bgmPlayer;
    QSoundEffect clearSound;

    void initUI();
    void clearBoard();
    void newPiece();
    bool tryMove(const Piece &newPiece, int newX, intY);
    void dropDown();
    bool oneLineDown();
    void pieceDropped();
    void removeFullLines();
    void drawBoard(QPainter &p);
    void drawPiece(QPainter &p, const Piece &piece, int x, int y);
};

#endif // TETRISWIDGET_H
