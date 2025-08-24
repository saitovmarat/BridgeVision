#ifndef FRAMEPROCESSOR_H
#define FRAMEPROCESSOR_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject> 
#include <QHostAddress>
#include <QMutex>
#include <QSize>
#include <opencv2/opencv.hpp>


static constexpr double MILLISECONDS_PER_SECOND = 1000.0;
static constexpr int DEFAULT_FRAME_DELAY_MS = 33;
static constexpr quint16 SERVER_PORT = 9999;
static constexpr const char* SERVER_HOST = "127.0.0.1";
inline static constexpr QSize SENT_IMAGE_SIZE(640, 480);
static constexpr int JPEG_QUALITY = 60;
static constexpr int SEND_BYTES_LIMIT = 65000;

class FrameProcessor : public QObject
{
    Q_OBJECT

public:
    explicit FrameProcessor(QString video_path, QObject *parent = nullptr);
    ~FrameProcessor() override;

signals:
    void frameReady(const QImage &image);
    void sendUdpDatagram(const QByteArray &data, const QHostAddress &addr, quint16 port);
    void playbackFinished();
    void errorOccurred(const QString &error);

public slots:
    void start();
    void processFrame();
    void stop();
    void setProcessingResult(const QJsonArray& detections, const QJsonObject& arch_center);

private:
    QString m_video_path;
    std::unique_ptr<cv::VideoCapture> m_capture;
    QTimer* m_timer = nullptr;
    int m_frame_delay_ms = DEFAULT_FRAME_DELAY_MS;
    bool m_stop = false;

    QJsonArray m_last_detections;
    QJsonObject m_last_arch_center; 
    mutable QMutex m_results_mutex;

    Q_DISABLE_COPY_MOVE(FrameProcessor)
};

#endif // FRAMEPROCESSOR_H
