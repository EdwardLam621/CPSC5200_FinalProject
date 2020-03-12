#include <QtWidgets>
#include <QtNetwork>
#include <QImage>
#include <QTextEdit>

#include <stdlib.h>

#include "server.h"
#include <qlocalserver.h>
#include <qlocalsocket.h>

Server::Server(QWidget *parent)
    : QDialog(parent)
{
    log = new QTextEdit;
    log->setReadOnly(true);
    image.fill(Qt::black);
//    statusLabel = new QLabel;
//    statusLabel->setWordWrap(true);
    quitButton = new QPushButton(tr("Quit"));
    quitButton->setAutoDefault(false);

    server = new QLocalServer(this);
    if (!server->listen("fortune")) {
        QMessageBox::critical(this, tr("Fortune Server"),
                              tr("Unable to start the server: %1.")
                              .arg(server->errorString()));
        close();
        return;
    }

    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(server, SIGNAL(newConnection()), this, SLOT(sendFortune()));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(quitButton);
    buttonLayout->addStretch(1);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(log);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Server"));

}

void Server::sendFortune()
{
    blockSize = 0;
    clientConnection = server->nextPendingConnection();
    connect(clientConnection, SIGNAL(readyRead()),this,SLOT(receiveImage()));
}

void Server::receiveImage(){
    QDataStream in(clientConnection);
    int cmd;
    QString ops;
    in.setVersion(QDataStream::Qt_4_0);
    if (blockSize == 0) {
        // Relies on the fact that QDataStream format streams a quint32 into sizeof(quint32) bytes
        if (clientConnection->bytesAvailable() < (int)sizeof(quint32))
            return;
        in >> cmd;
        if(cmd == 1)
            in >> ops;

        qDebug() << ops;

        for(int i=0;i<ops.size();i++){

            switch((int)(ops[i].unicode()-'0')){
            case 1:
            {
                i++;
                int dir = (int)(ops[i].unicode() -'0');
                image = flip(image,dir);
                if(dir == 0){
                    log->append("[process] flip horizon");
                }else{
                    log->append("[process] flip vertical");
                }
                break;
            }
            case 2:
                {
                i++;
                QString angle = "";
                for(;ops[i] != '#';i++){
                    angle += ops[i];
                }
                i++;
                image = rotate(image,angle.toInt());
                log->append("[process] rotate " + angle + " degree");
                break;
            }
            case 3:
                image = convertToGray(image);
                break;
            case 4:
                {
                i++;
                QString w = "";

                for(;ops[i] != '#';i++){
                    w += ops[i];
                }
                i++;
                QString h = "";
                for(;ops[i] != '#';i++){
                    h += ops[i];
                }
                i++;
                log->append("[process] resize from (" + QString::number(image.width()) +"x"+QString::number(image.height()) +") to (" + w + "x" + h + ")");
                image = resize(image,w.toInt(),h.toInt());
                break;
            }

            case 5:
                image = genThumbnail(image);
                break;
            default:
                break;
            }

        }
        in >> blockSize;
        qDebug() <<"in:"<< blockSize;
    }

    if(cmd == 0){
        if (clientConnection->bytesAvailable() < blockSize || in.atEnd())
            return;
        QByteArray array = clientConnection->read(blockSize);
        QBuffer buffer(&array);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer,"JPG");
        image = reader.read().copy();
    }else if(cmd == 1){
        QPixmap pic = QPixmap::fromImage(image);
        QBuffer _buffer;
        _buffer.open(QIODevice::ReadWrite);
        pic.save(&_buffer, "JPG");

        QByteArray dataStr;
        QDataStream out(&dataStr,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_0);
        out << (quint32) _buffer.data().size();
        qDebug() << "out:" << _buffer.data().size();
        connect(clientConnection, SIGNAL(disconnected()),
                clientConnection, SLOT(deleteLater()));
        dataStr.append(_buffer.data());
        clientConnection->write(dataStr);
        dataStr.resize(0);
        image.fill(Qt::black);
    }

}

// direction : 0 -> horizon , 1 -> vertical
QImage Server::flip(const QImage &img, int direction){
    if(direction == 0){
        return img.mirrored(true, false);
    }else if(direction == 1){
        return img.mirrored(false, true);
    }
    return img;
}

// degree : >0 -> clockwise, <0 -> anticlockwise
QImage Server::rotate(const QImage &img, float degree){
    QMatrix matrix;
    matrix.rotate(degree);
    return img.transformed(matrix,Qt::FastTransformation);

}

QImage Server::convertToGray(const QImage &img){
    QImage _img= img.copy();
    for (int ii = 0; ii < img.height(); ii++) {
        uchar* scan = _img.scanLine(ii);
        int depth =4;
        for (int jj = 0; jj < img.width(); jj++) {

            QRgb* rgbpixel = reinterpret_cast<QRgb*>(scan + jj*depth);
            int gray = qGray(*rgbpixel);
            *rgbpixel = QColor(gray, gray, gray).rgba();
        }
    }
    return _img;
}

QImage Server::resize(const QImage &img, int w, int h){
    return img.scaled(w,h);
}

QImage Server::genThumbnail(const QImage &img){
    return img.scaled(200, 150, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}
