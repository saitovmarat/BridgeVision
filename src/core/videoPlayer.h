// videoplayer.h
#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QUdpSocket>
#include <QImage>

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
    void frameReady(const QImage &image);        // Кадр готов к отображению
    void playbackFinished();
    void errorOccurred(const QString &error);

private slots:
    void onTimeout();
    void onUdpReadyRead();

private:
    QString m_filePath;
    bool m_isPlaying = false;
    void* m_capture = nullptr;
    QTimer* m_timer = nullptr;
    QUdpSocket* m_udpSocket = nullptr;

    double m_frameDelay = 33.0;

    // UDP
    QHostAddress m_serverAddress = QHostAddress("127.0.0.1");
    quint16 m_serverPort = 9999;     // Порт сервера
    quint16 m_localPort = 54322;    // Порт для приёма

    // Хранение текущего кадра
    QImage m_currentFrame;

    // Метод для рисования рамок
    QImage drawDetections(const QImage &image, const QJsonArray &detections, const QSize &sentSize);
};

#endif // VIDEORENDERER_H
