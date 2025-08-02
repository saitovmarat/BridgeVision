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
using namespace cv;

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
    VideoCapture* cap = new VideoCapture(filePath.toStdString());

    if (!cap->isOpened()) {
        emit errorOccurred("Не удалось открыть видео: " + filePath);
        delete cap;
        return false;
    }

    m_capture = static_cast<void*>(cap);

    double fps = cap->get(CAP_PROP_FPS);
    if (fps > 0) {
        m_frameDelay = 1000.0 / fps;
    } else {
        m_frameDelay = 33.0; // fallback: ~30 FPS
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

    if (m_capture) {
        VideoCapture* cap = static_cast<VideoCapture*>(m_capture);
        delete cap;
        m_capture = nullptr;
    }

    m_isPlaying = false;
}

void VideoPlayer::onTimeout()
{
    VideoCapture* cap = static_cast<VideoCapture*>(m_capture);
    if (!cap || !m_isPlaying) return;

    Mat frame;
    if (cap->read(frame)) {
        Mat rgb;
        cvtColor(frame, rgb, COLOR_BGR2RGB);

        QImage image(rgb.cols, rgb.rows, QImage::Format_RGB888);
        for (int i = 0; i < rgb.rows; ++i) {
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
        } else {
            qDebug() << "✅ Отправлено" << sent << "байт на" << m_serverAddress << m_serverPort;
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


        QImage processedImage = drawDetections(m_currentFrame, detections, QSize(640, 480));  // Sent size

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
    qDebug() << "Original size:" << originalImage.size();
    qDebug() << "Sent size:" << sentSize;

    if (originalImage.isNull() || detections.isEmpty()) {
        return originalImage;
    }

    QImage img = originalImage.copy();
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Параметры отрисовки
    QPen pen(Qt::green);
    pen.setWidth(4);
    pen.setCosmetic(true);  // Толщина не масштабируется
    painter.setPen(pen);

    QFont font = painter.font();
    font.setPointSize(14);
    font.setBold(true);
    painter.setFont(font);

    // Вычисляем, как originalImage масштабировался в sentSize
    QSizeF origSize(originalImage.width(), originalImage.height());

    // Сначала получаем scaledSize (без чёрных полос)
    QSizeF scaledSize = sentSize;
    scaledSize.scale(origSize, Qt::KeepAspectRatio);

    double scaleX = scaledSize.width() / origSize.width();
    double scaleY = scaledSize.height() / origSize.height();

    // Центрирование на сервере
    double offsetX = (sentSize.width() - scaledSize.width()) / 2.0;
    double offsetY = (sentSize.height() - scaledSize.height()) / 2.0;

    for (const QJsonValue &val : detections) {
        QJsonObject det = val.toObject();
        QString cls = det["class"].toString();
        double conf = det["confidence"].toDouble();
        int x1_s = det["x1"].toInt();
        int y1_s = det["y1"].toInt();
        int x2_s = det["x2"].toInt();
        int y2_s = det["y2"].toInt();

        // Убираем отступы (чёрные полосы)
        x1_s -= static_cast<int>(offsetX);
        y1_s -= static_cast<int>(offsetY);
        x2_s -= static_cast<int>(offsetX);
        y2_s -= static_cast<int>(offsetY);

        // Проверка: в пределах ли кадра сервера?
        if (x1_s >= scaledSize.width() || y1_s >= scaledSize.height() ||
            x2_s < 0 || y2_s < 0) {
            continue;
        }

        // Обрезаем
        x1_s = qMax(0, x1_s);
        y1_s = qMax(0, y1_s);
        x2_s = qMin(static_cast<int>(scaledSize.width()), x2_s);
        y2_s = qMin(static_cast<int>(scaledSize.height()), y2_s);

        // Масштабируем обратно на оригинал
        int x1_o = static_cast<int>(x1_s / scaleX);
        int y1_o = static_cast<int>(y1_s / scaleY);
        int x2_o = static_cast<int>(x2_s / scaleX);
        int y2_o = static_cast<int>(y2_s / scaleY);

        // Ограничиваем
        x1_o = qBound(0, x1_o, img.width() - 1);
        y1_o = qBound(0, y1_o, img.height() - 1);
        x2_o = qBound(0, x2_o, img.width() - 1);
        y2_o = qBound(0, y2_o, img.height() - 1);

        // Рисуем рамку
        painter.drawRect(x1_o, y1_o, x2_o - x1_o, y2_o - y1_o);

        // Подпись сверху
        QString label = QString("%1 (%2)").arg(cls).arg(conf, 0, 'f', 2);
        QRect textRect(x1_o, y1_o - 25, 150, 20);
        painter.fillRect(textRect, Qt::green);
        painter.setPen(Qt::white);
        painter.drawText(textRect, Qt::AlignCenter, label);
        painter.setPen(Qt::green);  // Восстанавливаем
    }

    painter.end();
    return img;
}
