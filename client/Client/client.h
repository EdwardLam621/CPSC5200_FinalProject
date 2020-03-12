#ifndef CLIENT_H
#define CLIENT_H

#include <QDialog>
#include <QList>
#include <qlocalsocket.h>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QLocalSocket;
class QTextEdit;
class QComboBox;
class QImage;
QT_END_NAMESPACE

class Client : public QDialog
{
    Q_OBJECT

public:
    Client(QWidget *parent = 0);

private slots:
    void upload();
    void process();
    void requestNewFortune(int _cmd = 0);
    void transImage();
    void readFortune();
    void displayError(QLocalSocket::LocalSocketError socketError);

    void uploadImage();
    void test();
    void flip();
    void rotate();
    void convertToGray();
    void resize();
    void genThumbnail();
    void save();
private:
    QPushButton *saveButton;
    QPushButton *uploadButton;
    QPushButton *processButton;
    QPushButton *testButton;
    QPushButton *flipButton;
    QPushButton *rotateButton;
    QPushButton *grayButton;
    QPushButton *resizeButton;
    QPushButton *thumbnailButton;
    QDialogButtonBox *buttonBox;
    QLocalSocket *socket;
    QString currentFortune;
    quint32 blockSize;
    QLabel *image;
    QString inputPath;
    int cmd;
    QString ops;
    bool uploaded;
    QString serverName;
    QTextEdit *log;
    QComboBox *flipDir;
    QLineEdit *resizeW;
    QLineEdit *resizeH;
    QLineEdit *rotateAngle;
    QImage out;
private:
    void reloadImage(const QImage &img);
    // direction : 0 -> left , 1 -> right
    QImage _flip(const QImage &img, int direction = 0);
    // degree : >0 -> right, <0 -> left
    QImage _rotate(const QImage &img, float degree = 90.0f);
    QImage _convertToGray(const QImage &img);
    QImage _resize(const QImage &img, int w, int h);
    QImage _genThumbnail(const QImage &img);

signals:
    void request(int);
};

#endif
