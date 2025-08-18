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

    bool loadVideo(const QString &file_path) override;
    void play() override;
    void stop() override;

private slots:
    void onUdpReadyRead();

private:
    QString m_file_path;
    QScopedPointer<FrameProcessor> m_processor;
    QScopedPointer<QThread> m_processing_thread;

    QUdpSocket* m_udp_socket;

    Q_DISABLE_COPY_MOVE(UdpLink)
};

#endif
