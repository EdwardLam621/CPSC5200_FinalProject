#include <QtWidgets>
#include <QtNetwork>
#include <QImage>
#include <QFrame>
#include <QList>
#include <QMessageBox>
#include <QTextEdit>
#include <qvalidator.h>

#include <climits>

#include "client.h"

Client::Client(QWidget *parent)
    : QDialog(parent),ops(""),uploaded(false),serverName("fortune"),out(nullptr)
{
    image = new QLabel;
    QPixmap p(640,360);
    image->setPixmap(p);

    log = new QTextEdit();
    log->setReadOnly(true);
    log->setFixedWidth(200);

    buttonBox = new QDialogButtonBox;

    saveButton = new QPushButton(tr("Save"));
    uploadButton = new QPushButton(tr("Upload"));
    processButton = new QPushButton(tr("Process"));

    buttonBox->addButton(uploadButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(processButton, QDialogButtonBox::ActionRole);

    flipButton = new QPushButton(tr("flip"));
    rotateButton = new QPushButton(tr("rotate"));
    grayButton = new QPushButton(tr("gray"));
    resizeButton = new QPushButton(tr("resize"));
    thumbnailButton = new QPushButton(tr("thumbnail"));

    socket = new QLocalSocket(this);

    flipDir = new QComboBox;
    flipDir->addItem("Horizon",0);
    flipDir->addItem("Vertical",1);

    QIntValidator *v = new QIntValidator( 0, INT_MAX, this );

    resizeW = new QLineEdit;
    resizeW->setValidator(v);
    resizeH = new QLineEdit;
    resizeH->setValidator(v);
    rotateAngle = new QLineEdit;
    rotateAngle->setValidator(v);

    connect(uploadButton, SIGNAL(clicked()),
             this,SLOT(uploadImage()));
    connect(processButton, SIGNAL(clicked()),
            this, SLOT(process()));

    connect(socket, SIGNAL(connected()),this,SLOT(transImage()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(readFortune()));
    connect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)),
            this, SLOT(displayError(QLocalSocket::LocalSocketError)));
    connect(flipButton,SIGNAL(clicked()),this,SLOT(flip()));
    connect(rotateButton,SIGNAL(clicked()),this,SLOT(rotate()));
    connect(grayButton,SIGNAL(clicked()),this,SLOT(convertToGray()));
    connect(resizeButton,SIGNAL(clicked()),this,SLOT(resize()));
    connect(thumbnailButton,SIGNAL(clicked()),this,SLOT(genThumbnail()));
    connect(this,SIGNAL(request(int)), this, SLOT(requestNewFortune(int)));
    connect(saveButton, SIGNAL(clicked(bool)),this,SLOT(save()));
    QGridLayout *panel = new QGridLayout(this);

    panel->addWidget(buttonBox,0,0,1,2);
    QLabel *l1 = new QLabel("flip direction");
    l1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    panel->addWidget(l1,0,2);
    panel->addWidget(flipDir,0,3);
    panel->addWidget(flipButton,0,4);
    QLabel *l2 = new QLabel("angle");
    l2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    panel->addWidget(l2,0,5);
    panel->addWidget(rotateAngle,0,6);
    panel->addWidget(rotateButton,0,7);
    panel->addWidget(grayButton,1,0);
    QLabel *l3 = new QLabel("width");
    l3->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    panel->addWidget(l3,1,1);
    panel->addWidget(resizeW,1,2);
    QLabel *l4 = new QLabel("height");
    l4->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    panel->addWidget(l4,1,3);
    panel->addWidget(resizeH,1,4);
    panel->addWidget(resizeButton,1,5);
    panel->addWidget(thumbnailButton,1,6);
    panel->addWidget(saveButton,1,7);

    panel->addWidget(image,2,0,1,6);
    panel->addWidget(log,2,7,1,1);
    setLayout(panel);

    setWindowTitle(tr("Client"));
}

void Client::requestNewFortune(int _cmd)
{
    blockSize = 0;
    cmd = _cmd;
    socket->abort();
    socket->connectToServer(serverName);
}

void Client::readFortune()
{
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_4_0);
    if (blockSize == 0) {

        // Relies on the fact that QDataStream format streams a quint32 into sizeof(quint32) bytes
        if (socket->bytesAvailable() < (int)sizeof(quint32))
            return;
        in >> blockSize;
        qDebug()<<"receive:" << blockSize;
    }

    if (socket->bytesAvailable() < blockSize || in.atEnd())
        return;

    QByteArray array = socket->read(blockSize);
    QBuffer buffer(&array);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer,"JPG");
    QImage _image = reader.read();
    reloadImage(_image);
    out = _image.copy();
    log->setTextColor(Qt::green);
    log->append("[Operation] Finish");
    log->setTextColor(Qt::black);
    qDebug() << "ok";
}

void Client::displayError(QLocalSocket::LocalSocketError socketError)
{
    switch (socketError) {
    case QLocalSocket::ServerNotFoundError:
        QMessageBox::information(this, tr("Fortune Client"),
                                 tr("The host was not found. Please check the "
                                    "host name and port settings."));
        break;
    case QLocalSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("Fortune Client"),
                                 tr("The connection was refused by the peer. "
                                    "Make sure the fortune server is running, "
                                    "and check that the host name and port "
                                    "settings are correct."));
        break;
    case QLocalSocket::PeerClosedError:
        break;
    default:
        QMessageBox::information(this, tr("Fortune Client"),
                                 tr("The following error occurred: %1.")
                                 .arg(socket->errorString()));
    }
}


void Client::uploadImage(){
    QString str_path = QFileDialog::getOpenFileName(this, tr("choose an image file"), tr("/home"), tr("(*.*);;"));
    if(!str_path.isEmpty()){
        QImage *img = new QImage;
        img->load(str_path);
        reloadImage(*img);
        inputPath = str_path;
        emit request(0);
        uploaded = true;
        log->append("[File] Choose " + str_path);
    }
}

void Client::reloadImage(const QImage &img){
    image->setPixmap(QPixmap::fromImage(img));
    this->adjustSize();
}
// direction : 0 -> horizon , 1 -> vertical
QImage Client::_flip(const QImage &img, int direction){
    if(direction == 0){
        return img.mirrored(true, false);
    }else if(direction == 1){
        return img.mirrored(false, true);
    }
    return img;
}

// degree : >0 -> clockwise, <0 -> anticlockwise
QImage Client::_rotate(const QImage &img, float degree){
    QMatrix matrix;
    matrix.rotate(degree);
    return img.transformed(matrix,Qt::FastTransformation);

}

QImage Client::_convertToGray(const QImage &img){
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

QImage Client::_resize(const QImage &img, int w, int h){
    return img.scaled(w,h);
}

QImage Client::_genThumbnail(const QImage &img){
    return img.scaled(200, 150, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

void Client::test(){
}

void Client::flip(){
    if(flipDir->currentData().toInt() == 0)
        log->append("[Operation] horizon flip");
    else
        log->append("[Operation] vertical flip");
    ops += "1";
    ops += flipDir->currentData().toString();

}

void Client::rotate(){
    log->append("[Operation] rotate " + rotateAngle->text() + " degree");
    ops+="2";
    ops += rotateAngle->text();
    ops += "#";
}

void Client::convertToGray(){
    log->append("[Operation] convert to gray");
    ops+="3";
}

void Client::resize(){
    log->append("[Operation] resize from (" +
                QString::number(out.width()) + "x" + QString::number(out.height()) +
                ") to (" +
                resizeW->text() + "x" + resizeH->text() +")");
    ops+="4";
    ops+= resizeW->text();
    ops += "#";
    ops+= resizeW->text();
    ops += "#";
}

void Client::genThumbnail(){
    log->append("[Operation] generate thumbnail");
    ops+="5";
}

void Client::transImage(){
    QPixmap pix(inputPath);
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    pix.save(&buffer,"jpg");
    out = pix.toImage();
    QByteArray dataArray;
    QDataStream out(&dataArray,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << (quint32) (cmd);
    if(cmd == 0){
        out << (quint32) buffer.data().size();
        qDebug() << "trans:"<<buffer.data().size();
        dataArray.append(buffer.data());
    }else{
        out << ops;
        out << (quint32) (0);
    }
    socket->write(dataArray);
    log->append("[File] Upload " + inputPath);
    dataArray.resize(0);
    ops.clear();
}
void Client::upload(){
    emit request(0);
}

void Client::process(){
    if(uploaded){
        emit request(1);
        uploaded = false;
    }else{
        log->setTextColor(Qt::red);
        log->append("Have not loaded the file!");
        log->setTextColor(Qt::black);
        QMessageBox::warning(NULL, "NOT LOAD!",
                             "Load the file processed first, please!", QMessageBox::Ok, QMessageBox::Ok);
        ops.clear();
        flipDir->clear();
        resizeW->clear();
        resizeH->clear();
    }
}

void Client::save(){
    QString fileName = QFileDialog::getSaveFileName(this,tr("Save Image"),"",tr("Images (*.png *.bmp *.jpg)"));
    if(!out.isNull())
        out.save(fileName);
}
