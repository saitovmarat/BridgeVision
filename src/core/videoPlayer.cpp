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
#include <chrono>
#include <thread>

#include <opencv2/opencv.hpp>

VideoPlayer::VideoPlayer(QObject *parent)
    : QObject(parent)
    , m_udpSocket(new QUdpSocket(this))
{
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
    if (m_isPlaying || !m_capture || !m_capture->isOpened()) return;

    m_isPlaying = true;
    m_stopProcessing = false;

    m_processingThread = std::thread(&VideoPlayer::processFrames, this);
}

void VideoPlayer::stop()
{
    m_isPlaying = false;
    m_stopProcessing = true;

    if (m_processingThread.joinable()) {
        m_processingThread.join();
    }

    m_capture.reset();
    m_frameQueue.clear();
}

void VideoPlayer::processFrames()
{
    auto frameDelay = std::chrono::milliseconds(static_cast<long long>(m_frameDelay));

    while (m_isPlaying && !m_stopProcessing) {
        auto start = std::chrono::steady_clock::now();

        cv::Mat frame;
        if (!m_capture->read(frame)) {
            emit playbackFinished();
            break;
        }

        cv::Mat rgb;
        cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

        QImage image(rgb.cols, rgb.rows, QImage::Format_RGB888);
        for (int i = 0; i < rgb.rows; ++i) {
            memcpy(image.scanLine(i), rgb.ptr(i), rgb.cols * 3);
        }

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
        } else {
            m_udpSocket->writeDatagram(data, m_serverAddress, m_serverPort);
        }

        QImage processedImage = image;
        {
            std::lock_guard<std::mutex> lock(m_detectionsMutex);
            if (m_hasValidDetections) {
                processedImage = drawDetections(image, m_lastDetections, QSize(640, 480));
            }
        }

        if (!processedImage.isNull()) {
            emit frameReady(processedImage);
        }

        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        auto sleepTime = frameDelay - elapsed;

        if (sleepTime > std::chrono::milliseconds::zero()) {
            std::this_thread::sleep_for(sleepTime);
        }
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

        std::lock_guard<std::mutex> lock(m_detectionsMutex);
        m_lastDetections = detections;
        m_hasValidDetections = true;
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

    cv::Mat mat = QImageToCvMat(originalImage);

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

        x1_o = std::max(0, x1_o);
        y1_o = std::max(0, y1_o);
        x2_o = std::min(mat.cols - 1, x2_o);
        y2_o = std::min(mat.rows - 1, y2_o);

        cv::rectangle(mat, cv::Rect(x1_o, y1_o, x2_o - x1_o, y2_o - y1_o),
                      cv::Scalar(0, 255, 0), 2);

        QString label = QString("%1 (%2)").arg(cls).arg(conf, 0, 'f', 2);
        std::string text = label.toStdString();

        cv::Point org(x1_o, y1_o - 5);
        cv::putText(mat, text, org, cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    cv::Scalar(0, 255, 0), 2);
    }

    return CvMatToQImage(mat);
}


cv::Mat QImageToCvMat(const QImage &image)
{
    if (image.isNull()) {
        return cv::Mat();
    }

    cv::Mat mat;
    switch (image.format()) {
    case QImage::Format_RGB888:
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, const_cast<uchar*>(image.bits()), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
        break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGR);
        break;
    default:
        qWarning() << "Unsupported QImage format:" << image.format();
        return cv::Mat();
    }
    return mat;
}

QImage CvMatToQImage(const cv::Mat &mat)
{
    if (mat.empty()) {
        return QImage();
    }

    cv::Mat rgb;
    switch (mat.type()) {
    case CV_8UC3:
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        break;
    case CV_8UC4:
        cv::cvtColor(mat, rgb, cv::COLOR_BGRA2RGB);
        break;
    default:
        qWarning() << "Unsupported cv::Mat type:" << mat.type();
        return QImage();
    }

    QImage image(rgb.cols, rgb.rows, QImage::Format_RGB888);
    for (int i = 0; i < rgb.rows; ++i) {
        memcpy(image.scanLine(i), rgb.ptr(i), rgb.cols * 3);
    }

    return image;
}
