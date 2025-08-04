#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QUdpSocket>
#include <QImage>
#include <QQueue>
#include <QJsonArray>

#include <opencv2/opencv.hpp>

class VideoPlayer : public QObject
{
    Q_OBJECT

public:
    explicit VideoPlayer(QObject *parent = nullptr);
    ~VideoPlayer();

    bool loadVideo(const QString &filePath);
    void play();
    void stop();

signals:
    void frameReady(const QImage &image);
    void playbackFinished();
    void errorOccurred(const QString &error);

private slots:
    void onUdpReadyRead();

private:
    void processFrames();
    QImage drawDetections(const QImage &originalImage,
                          const QJsonArray &detections,
                          const QSize &sentSize);

    QString m_filePath;
    bool m_isPlaying = false;
    bool m_stopProcessing = true;
    std::unique_ptr<cv::VideoCapture> m_capture;
    QUdpSocket* m_udpSocket = nullptr;

    double m_frameDelay = 33.0;

    QHostAddress m_serverAddress = QHostAddress("127.0.0.1");
    quint16 m_serverPort = 9999;
    quint16 m_localPort = 54322;

    QImage m_currentFrame;
    QQueue<QImage> m_frameQueue;
    static constexpr int MAX_QUEUE_SIZE = 10;

    std::thread m_processingThread;
    mutable std::mutex m_framesMutex;

    QJsonArray m_lastDetections;
    bool m_hasValidDetections = false;
    mutable std::mutex m_detectionsMutex;

};

#endif // VIDEORENDERER_H
