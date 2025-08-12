#ifndef FRAMEPROCESSOR_H
#define FRAMEPROCESSOR_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QHostAddress>
#include <QMutex>

#include <opencv2/opencv.hpp>

class FrameProcessor : public QObject
{
    Q_OBJECT

public:
    explicit FrameProcessor(const QString &videoPath, QObject *parent = nullptr);
    ~FrameProcessor();

signals:
    void frameReady(const QImage &image);
    void sendUdpDatagram(const QByteArray &data, const QHostAddress &addr, quint16 port);
    void playbackFinished();
    void errorOccurred(const QString &error);

public slots:
    void start();
    void processFrame();
    void stop();
    void setDetections(const QJsonArray &detections);

private:
    QString m_videoPath;
    std::unique_ptr<cv::VideoCapture> m_capture;
    QTimer* m_timer = nullptr;
    int m_frameDelayMs = 33;
    bool m_stop = false;

    QMutex m_detectionsMutex;
    QJsonArray m_lastDetections;
    bool m_hasValidDetections = false;
};

#endif // FRAMEPROCESSOR_H
