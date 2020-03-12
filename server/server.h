#ifndef SERVER_H
#define SERVER_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QLocalServer;
class QLocalSocket;
class QImage;
class QTextEdit;
QT_END_NAMESPACE

class Server : public QDialog
{
    Q_OBJECT

public:
    Server(QWidget *parent = 0);

private slots:
    void sendFortune();
    void receiveImage();
private:
    QLabel *statusLabel;
    QPushButton *quitButton;
    QLocalServer *server;
    QStringList fortunes;
    int blockSize;
    QLocalSocket *clientConnection;
    QImage image;
    QTextEdit *log;
private:
    // direction : 0 -> left , 1 -> right
    QImage flip(const QImage &img, int direction = 0);
    // degree : >0 -> right, <0 -> left
    QImage rotate(const QImage &img, float degree = 90.0f);
    QImage convertToGray(const QImage &img);
    QImage resize(const QImage &img, int w, int h);
    QImage genThumbnail(const QImage &img);
    //void displayError(QLocalSocket::LocalSocketError socketError);
};

#endif
