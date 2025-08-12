#ifndef UDPLINK_H
#define UDPLINK_H

#include "iLink.h"

#include <QObject>
#include <QHostAddress>
#include <QScopedPointer>

class QUdpSocket;
class FrameProcessor;

class UdpLink : public ILink
{
    Q_OBJECT

public:
    explicit UdpLink(QObject *parent = nullptr);
    ~UdpLink() override;

    bool loadVideo(const QString &filePath) override;
    void play() override;
    void stop() override;

private slots:
    void onUdpReadyRead();

private:
    QString m_filePath;
    QScopedPointer<FrameProcessor> m_processor;
    QScopedPointer<QThread> m_processingThread;

    QUdpSocket* m_udpSocket;
};

#endif
