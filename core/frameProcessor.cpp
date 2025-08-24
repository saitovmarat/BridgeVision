#include "frameProcessor.h"
#include "overlayRenderer.h"
#include "imageConverter.h"

#include <qobjectdefs.h>       
#include <QDebug>               
#include <QObject>
#include <utility>
#include <memory>
#include <cstring>
#include <QImage>
#include <Qt>
#include <QString>
#include <QTimer>
#include <QByteArray>
#include <QBuffer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QHostAddress>
#include <QMutexLocker>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>


FrameProcessor::FrameProcessor(QString video_path, QObject *parent)
    : QObject(parent)
    , m_video_path(std::move(video_path)) {}

FrameProcessor::~FrameProcessor() = default;

void FrameProcessor::start()
{
    if (m_capture) {
        emit errorOccurred("Видео уже открыто");
        return;
    }

    m_capture = std::make_unique<cv::VideoCapture>(m_video_path.toStdString());
    if (!m_capture->isOpened()) {
        emit errorOccurred("Не удалось открыть видео: " + m_video_path);
        return;
    }

    const double fps = m_capture->get(cv::CAP_PROP_FPS);
    m_frame_delay_ms = (fps > 0) ? static_cast<int>(MILLISECONDS_PER_SECOND / fps) : DEFAULT_FRAME_DELAY_MS;

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &FrameProcessor::processFrame);
    m_timer->start(m_frame_delay_ms);

    qDebug() << "FrameProcessor: старт воспроизведения. FPS:" << (MILLISECONDS_PER_SECOND / m_frame_delay_ms);
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
    if (m_stop) { return; }

    cv::Mat frame;
    if (!m_capture->read(frame)) {
        emit playbackFinished();
        stop();
        return;
    }

    const QImage image = cvMatToQImage(frame);
    const QImage small_image = image.scaled(SENT_IMAGE_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QByteArray buffer;
    QBuffer qbuffer(&buffer);
    qbuffer.open(QIODevice::WriteOnly);
    small_image.save(&qbuffer, "JPEG", JPEG_QUALITY);
    qbuffer.close();

    const QString base64 = buffer.toBase64();

    QJsonObject json;
    json["frame"] = base64;
    const QJsonDocument doc(json);
    const QByteArray datagram = doc.toJson(QJsonDocument::Compact);

    if (datagram.size() > SEND_BYTES_LIMIT) {
        emit errorOccurred("UDP-пакет слишком большой: " + QString::number(datagram.size()) + " байт");
    } else {
        emit sendUdpDatagram(datagram, QHostAddress(SERVER_HOST), SERVER_PORT);
    }

    QImage processed_image = image;
    {
        const QMutexLocker locker(&m_results_mutex);
        if (!m_last_detections.isEmpty()) {
            processed_image = drawDetections(image, m_last_detections, SENT_IMAGE_SIZE);
        }
        if (!m_last_arch_center.isEmpty()) {
            processed_image = drawArchCenter(processed_image, m_last_arch_center, SENT_IMAGE_SIZE);
        }
    }

    emit frameReady(processed_image);
}

void FrameProcessor::setProcessingResult(const QJsonArray& detections, const QJsonObject& arch_center)
{
    const QMutexLocker locker(&m_results_mutex);
    m_last_detections = detections;
    m_last_arch_center = arch_center;
}