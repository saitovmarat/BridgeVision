#ifndef VIDEOP_LAYER_H
#define VIDEOP_LAYER_H

#include <QObject>
#include <QHostAddress>
#include <QScopedPointer>

class QUdpSocket;
class FrameProcessor;

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
    void errorOccurred(const QString &error);
    void playbackFinished();
    void detectionsReceived(const QJsonArray &detections);

private slots:
    void onUdpReadyRead();

private:
    QString m_filePath;
    QScopedPointer<FrameProcessor> m_processor;
    QScopedPointer<QThread> m_processingThread;

    QUdpSocket *m_udpSocket;
    QHostAddress m_serverAddress = QHostAddress("127.0.0.1");
    quint16 m_serverPort = 9999;
};

#endif // VIDEORENDERER_H
