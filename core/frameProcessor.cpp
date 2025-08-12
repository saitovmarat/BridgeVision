#include "frameProcessor.h"
#include "detectionsDrawer.h"

#include <QCoreApplication>
#include <QThread>

#include <QImage>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QFileInfo>
#include <QTimer>

#include <opencv2/opencv.hpp>

static constexpr quint16 SERVER_PORT = 9999;
static constexpr const char* SERVER_HOST = "127.0.0.1";
static const QSize SENT_IMAGE_SIZE(640, 480);
static constexpr int JPEG_QUALITY = 60;

FrameProcessor::FrameProcessor(const QString &videoPath, QObject *parent)
    : QObject(parent)
    , m_videoPath(videoPath) {}

FrameProcessor::~FrameProcessor() = default;

void FrameProcessor::start()
{
    if (m_capture) {
        emit errorOccurred("Видео уже открыто");
        return;
    }

    m_capture = std::make_unique<cv::VideoCapture>(m_videoPath.toStdString());
    if (!m_capture->isOpened()) {
        emit errorOccurred("Не удалось открыть видео: " + m_videoPath);
        return;
    }

    double fps = m_capture->get(cv::CAP_PROP_FPS);
    m_frameDelayMs = (fps > 0) ? static_cast<int>(1000.0 / fps) : 33;

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &FrameProcessor::processFrame);
    m_timer->start(m_frameDelayMs);

    qDebug() << "FrameProcessor: старт воспроизведения. FPS:" << (1000.0 / m_frameDelayMs);
}

void FrameProcessor::stop()
{
    m_stop = true;

    if (m_timer) {
        m_timer->stop();
    }

    m_capture.reset();
}

void FrameProcessor::processFrame()
{
    if (m_stop) return;

    cv::Mat frame;
    if (!m_capture->read(frame)) {
        emit playbackFinished();
        stop();
        return;
    }

    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

    QImage image(rgb.cols, rgb.rows, QImage::Format_RGB888);
    for (int i = 0; i < rgb.rows; ++i) {
        memcpy(image.scanLine(i), rgb.ptr(i), rgb.cols * 3);
    }

    QImage smallImage = image.scaled(SENT_IMAGE_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QByteArray buffer;
    QBuffer qbuffer(&buffer);
    qbuffer.open(QIODevice::WriteOnly);
    smallImage.save(&qbuffer, "JPEG", JPEG_QUALITY);
    qbuffer.close();

    QString base64 = buffer.toBase64();

    QJsonObject json;
    json["frame"] = base64;
    QJsonDocument doc(json);
    QByteArray datagram = doc.toJson(QJsonDocument::Compact);

    if (datagram.size() > 65000) {
        emit errorOccurred("UDP-пакет слишком большой: " + QString::number(datagram.size()) + " байт");
    } else {
        emit sendUdpDatagram(datagram, QHostAddress(SERVER_HOST), SERVER_PORT);
    }

    QImage processedImage = image;
    {
        QMutexLocker locker(&m_detectionsMutex);
        if (m_hasValidDetections && !m_lastDetections.isEmpty()) {
            processedImage = drawDetections(image, m_lastDetections, SENT_IMAGE_SIZE);
        }
    }

    emit frameReady(processedImage);
}

void FrameProcessor::setDetections(const QJsonArray &detections)
{
    QMutexLocker locker(&m_detectionsMutex);
    m_lastDetections = detections;
    m_hasValidDetections = true;
}
