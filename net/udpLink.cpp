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
    , m_udp_socket(new QUdpSocket(this))
{
    if (!m_udp_socket->bind(QHostAddress::Any, 0)) {
        const QString error = m_udp_socket->errorString();
        qDebug() << "Error bind():" << m_udp_socket->error() << error;
    } else {
        qDebug() << "UDP socket is listening on the port:" << m_udp_socket->localPort();;
        connect(m_udp_socket, &QUdpSocket::readyRead, this, &UdpLink::onUdpReadyRead);
    }
}

UdpLink::~UdpLink() = default;

bool UdpLink::loadVideo(const QString &file_path)
{
    stop();

    if (file_path.isEmpty()) {
        emit errorOccurred("Filepath to the video is not set!");
        return false;
    }

    const QFileInfo info(file_path);
    if (!info.exists()) {
        emit errorOccurred("File does not exist: " + file_path);
        return false;
    }

    m_file_path = file_path;
    return true;
}

void UdpLink::play()
{
    if (m_processing_thread || m_file_path.isEmpty()) {
        return;
    }

    m_processor.reset(new FrameProcessor(m_file_path));
    m_processing_thread.reset(new QThread(this));

    m_processor->moveToThread(m_processing_thread.data());

    connect(m_processor.data(), &FrameProcessor::frameReady, this, &UdpLink::frameReady);
    connect(m_processor.data(), &FrameProcessor::errorOccurred, this, &UdpLink::errorOccurred);
    connect(m_processor.data(), &FrameProcessor::playbackFinished, this, &UdpLink::playbackFinished);

    connect(m_processor.data(), &FrameProcessor::sendUdpDatagram, this, [this](
            const QByteArray &data, const QHostAddress &addr, quint16 port) {
        m_udp_socket->writeDatagram(data, addr, port);
    }, Qt::QueuedConnection);

    connect(m_processing_thread.data(), &QThread::started, this, [this]() {
        QMetaObject::invokeMethod(m_processor.data(), &FrameProcessor::start, Qt::QueuedConnection); // NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
    });

    connect(this, &UdpLink::processingResultReceived, m_processor.data(), &FrameProcessor::setProcessingResult, Qt::QueuedConnection);

    m_processing_thread->start();
}

void UdpLink::stop()
{
    if (m_processing_thread) {
        QMetaObject::invokeMethod(m_processor.data(), &FrameProcessor::stop, Qt::BlockingQueuedConnection);

        m_processing_thread->quit();
        m_processing_thread->wait();

        m_processing_thread.reset();
        m_processor.reset();
    }
}

void UdpLink::onUdpReadyRead()
{
    while (m_udp_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udp_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 sender_port;

        m_udp_socket->readDatagram(datagram.data(), datagram.size(), &sender, &sender_port);

        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(datagram, &error);
        if (error.error != QJsonParseError::NoError) {
            emit errorOccurred("JSON parse error: " + error.errorString());
            continue;
        }

        QJsonObject response = doc.object();

        if (response.contains("error")) {
            emit errorOccurred("Server error: " + response["error"].toString());
            continue;
        }

        const QJsonArray detections = response["detections"].toArray();
        const QJsonObject target_point = response["target_point"].toObject();

        emit processingResultReceived(detections, target_point);
    }
}

