// videoplayer.cpp
#include "videoplayer.h"
#include <QDebug>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>
#include <QPen>
#include <QFont>

#include <opencv2/opencv.hpp>

VideoPlayer::VideoPlayer(QObject *parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &VideoPlayer::onTimeout);

    m_udpSocket = new QUdpSocket(this);
    if (!m_udpSocket->bind(QHostAddress::Any, m_localPort)) {
        emit errorOccurred("Не удалось забиндить UDP-сокет: " + m_udpSocket->errorString());
    } else {
        connect(m_udpSocket, &QUdpSocket::readyRead, this, &VideoPlayer::onUdpReadyRead);
        qDebug() << "UDP-сокет слушает на порту:" << m_localPort;
    }
}

VideoPlayer::~VideoPlayer()
{
    stop();
}

bool VideoPlayer::loadVideo(const QString &filePath)
{
    stop();

    m_filePath = filePath;
    m_capture = std::make_unique<cv::VideoCapture>(filePath.toStdString());

    if (!m_capture->isOpened()) {
        emit errorOccurred("Не удалось открыть видео: " + filePath);
        return false;
    }

    double fps = m_capture->get(cv::CAP_PROP_FPS);
    if (fps > 0) {
        m_frameDelay = 1000.0 / fps;
    } else {
        m_frameDelay = 33.0;
    }

    qDebug() << "Видео загружено. FPS:" << fps << "Задержка:" << m_frameDelay << "мс";

    return true;
}

void VideoPlayer::play()
{
    if (m_isPlaying || !m_capture) return;

    m_isPlaying = true;
    m_timer->start(static_cast<int>(m_frameDelay));
}

void VideoPlayer::stop()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }

    m_capture.reset();
    m_isPlaying = false;
}

void VideoPlayer::onTimeout()
{
    if (!m_capture || !m_isPlaying) return;

    cv::Mat frame;
    if (m_capture->read(frame)) {
        cv::Mat rgb;
        cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

        QImage image(rgb.cols, rgb.rows, QImage::Format_RGB888);
        for (int i = 0; i < rgb.rows; i++) {
            memcpy(image.scanLine(i), rgb.ptr(i), rgb.cols * 3);
        }

        m_currentFrame = image;

        const QSize targetSize(640, 480);
        QImage smallImage = image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QByteArray buffer;
        QBuffer qbuffer(&buffer);
        qbuffer.open(QIODevice::WriteOnly);
        smallImage.save(&qbuffer, "JPEG", 60);
        qbuffer.close();

        QString b64 = buffer.toBase64();

        QJsonObject json;
        json["frame"] = b64;
        QJsonDocument doc(json);
        QByteArray data = doc.toJson(QJsonDocument::Compact);

        if (data.size() > 65000) {
            emit errorOccurred("UDP-пакет слишком большой: " + QString::number(data.size()));
            return;
        }

        qint64 sent = m_udpSocket->writeDatagram(data, m_serverAddress, m_serverPort);
        if (sent == -1) {
            emit errorOccurred("Ошибка отправки UDP: " + m_udpSocket->errorString());
        }
    } else {
        stop();
        emit playbackFinished();
    }
}

void VideoPlayer::onUdpReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(datagram, &error);
        if (error.error != QJsonParseError::NoError) {
            emit errorOccurred("JSON parse error: " + error.errorString());
            return;
        }

        QJsonObject response = doc.object();
        if (response.contains("error")) {
            emit errorOccurred("Серверная ошибка: " + response["error"].toString());
            return;
        }

        if (!response["success"].toBool()) {
            emit errorOccurred("Обработка на сервере не удалась");
            return;
        }

        QJsonArray detections = response["detections"].toArray();

        QImage processedImage = drawDetections(m_currentFrame, detections, QSize(640, 480));
        if (!processedImage.isNull()) {
            emit frameReady(processedImage);
        }
    }
}

QImage VideoPlayer::drawDetections(
    const QImage &originalImage,
    const QJsonArray &detections,
    const QSize &sentSize)
{
    if (originalImage.isNull() || detections.isEmpty()) {
        return originalImage;
    }

    QImage img = originalImage.copy();
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QPen pen(Qt::green);
    pen.setWidth(4);
    painter.setPen(pen);

    QFont font = painter.font();
    font.setPointSize(14);
    font.setBold(true);
    painter.setFont(font);

    QSizeF origSize(originalImage.width(), originalImage.height());

    QSizeF scaledSize = origSize;
    scaledSize.scale(sentSize, Qt::KeepAspectRatio);

    double scaleBackX = origSize.width() / scaledSize.width();
    double scaleBackY = origSize.height() / scaledSize.height();

    for (const QJsonValue &val : detections) {
        QJsonObject det = val.toObject();
        QString cls = det["class"].toString();
        double conf = det["confidence"].toDouble();
        int x1_s = det["x1"].toInt();
        int y1_s = det["y1"].toInt();
        int x2_s = det["x2"].toInt();
        int y2_s = det["y2"].toInt();

        int x1_o = static_cast<int>(x1_s * scaleBackX);
        int y1_o = static_cast<int>(y1_s * scaleBackY);
        int x2_o = static_cast<int>(x2_s * scaleBackX);
        int y2_o = static_cast<int>(y2_s * scaleBackY);

        x1_o = qBound(0, x1_o, img.width() - 1);
        y1_o = qBound(0, y1_o, img.height() - 1);
        x2_o = qBound(0, x2_o, img.width() - 1);
        y2_o = qBound(0, y2_o, img.height() - 1);

        painter.drawRect(x1_o, y1_o, x2_o - x1_o, y2_o - y1_o);

        QString label = QString("%1 (%2)").arg(cls).arg(conf, 0, 'f', 2);
        QRect textRect(x1_o, y1_o - 25, 150, 20);
        painter.fillRect(textRect, Qt::green);
        painter.setPen(Qt::white);
        painter.drawText(textRect, Qt::AlignCenter, label);
        painter.setPen(Qt::green);
    }

    painter.end();
    return img;
}
