#include "udpLink.h"
#include "frameProcessor.h"

#include <QDebug>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaObject>
#include <QThread>
#include <QScopedPointer>

#include <QUdpSocket>

#include <opencv2/opencv.hpp>

UdpLink::UdpLink(QObject *parent)
    : ILink(parent)
    , m_udpSocket(new QUdpSocket(this))
{
    if (!m_udpSocket->bind(QHostAddress::Any, 0)) {
        QString error = m_udpSocket->errorString();
        qDebug() << "Ошибка bind():" << m_udpSocket->error() << error;
    } else {
        qDebug() << "UDP-сокет слушает на порту:" << m_udpSocket->localPort();;
        connect(m_udpSocket, &QUdpSocket::readyRead, this, &UdpLink::onUdpReadyRead);
    }
}

UdpLink::~UdpLink()
{
    stop();
}

bool UdpLink::loadVideo(const QString &filePath)
{
    stop();

    if (filePath.isEmpty()) {
        emit errorOccurred("Путь к видео не указан");
        return false;
    }

    QFileInfo info(filePath);
    if (!info.exists()) {
        emit errorOccurred("Файл не существует: " + filePath);
        return false;
    }

    m_filePath = filePath;
    return true;
}

void UdpLink::play()
{
    if (m_processingThread || m_filePath.isEmpty()) {
        return;
    }

    m_processor.reset(new FrameProcessor(m_filePath));
    m_processingThread.reset(new QThread(this));

    m_processor->moveToThread(m_processingThread.data());

    connect(m_processor.data(), &FrameProcessor::frameReady, this, &UdpLink::frameReady);
    connect(m_processor.data(), &FrameProcessor::errorOccurred, this, &UdpLink::errorOccurred);
    connect(m_processor.data(), &FrameProcessor::playbackFinished, this, &UdpLink::playbackFinished);

    connect(m_processor.data(), &FrameProcessor::sendUdpDatagram, this, [this](
            const QByteArray &data, const QHostAddress &addr, quint16 port) {
        m_udpSocket->writeDatagram(data, addr, port);
    }, Qt::QueuedConnection);

    connect(m_processingThread.data(), &QThread::started, this, [this]() {
        QMetaObject::invokeMethod(m_processor.data(), &FrameProcessor::start, Qt::QueuedConnection);
    });

    connect(this, &UdpLink::detectionsReceived, m_processor.data(), &FrameProcessor::setDetections, Qt::QueuedConnection);

    m_processingThread->start();
}

void UdpLink::stop()
{
    if (m_processingThread) {
        QMetaObject::invokeMethod(m_processor.data(), &FrameProcessor::stop, Qt::BlockingQueuedConnection);

        m_processingThread->quit();
        m_processingThread->wait();

        m_processingThread.reset();
        m_processor.reset();
    }
}

void UdpLink::onUdpReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(datagram, &error);
        if (error.error != QJsonParseError::NoError) {
            emit errorOccurred("JSON parse error: " + error.errorString());
            continue;
        }

        QJsonObject response = doc.object();

        if (response.contains("error")) {
            emit errorOccurred("Серверная ошибка: " + response["error"].toString());
            continue;
        }

        if (!response["success"].toBool()) {
            emit errorOccurred("Обработка на сервере не удалась");
            continue;
        }

        QJsonArray detections = response["detections"].toArray();

        emit detectionsReceived(detections);
    }
}

