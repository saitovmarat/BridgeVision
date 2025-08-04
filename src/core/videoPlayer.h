#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QUdpSocket>
#include <QImage>

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
    void onTimeout();
    void onUdpReadyRead();

private:
    QString m_filePath;
    bool m_isPlaying = false;
    std::unique_ptr<cv::VideoCapture> m_capture;
    QTimer* m_timer;
    QUdpSocket* m_udpSocket;

    double m_frameDelay = 33.0;

    QHostAddress m_serverAddress = QHostAddress("127.0.0.1");
    quint16 m_serverPort = 9999;
    quint16 m_localPort = 54322;

    QImage m_currentFrame;

    QImage drawDetections(const QImage &image, const QJsonArray &detections, const QSize &sentSize);
};

#endif // VIDEORENDERER_H
