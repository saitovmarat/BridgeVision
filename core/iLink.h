#ifndef ILINK_H
#define ILINK_H

#include <QObject>
#include <QImage>


class ILink : public QObject
{
    Q_OBJECT

public:
    explicit ILink(QObject *parent = nullptr) : QObject(parent) {}
    ~ILink() override = default;

    virtual bool loadVideo(const QString &file_path) = 0;
    virtual void play() = 0;
    virtual void stop() = 0;

signals:
    void frameReady(const QImage &image);
    void errorOccurred(const QString &error);
    void playbackFinished();
    void processingResultReceived(
        const QJsonArray &detections,
        const QJsonObject &arch_center
    );

private:
    Q_DISABLE_COPY_MOVE(ILink)
};

#endif
