#include "detectionsDrawer.h"
#include "imageConverter.h"

#include <QPainter>
#include <QPen>
#include <QFont>
#include <QDebug>
#include <QJsonObject>

#include <opencv2/opencv.hpp>


QImage drawDetections(
    const QImage &originalImage,
    const QJsonArray &detections,
    const QSize &sentSize)
{
    if (originalImage.isNull() || detections.isEmpty()) {
        return originalImage;
    }

    cv::Mat mat = QImageToCvMat(originalImage);

    QSizeF origSize(originalImage.width(), originalImage.height());
    QSizeF scaledSize = origSize;
    scaledSize.scale(sentSize, Qt::KeepAspectRatio);

    double scaleBackX = origSize.width() / scaledSize.width();
    double scaleBackY = origSize.height() / scaledSize.height();

    for (const QJsonValue &val : detections) {
        QJsonObject det = val.toObject();
        QString cls = det["class"].toString();
        double conf = det["confidence"].toDouble();
        int x1_s = det["x1"].toInt();
        int y1_s = det["y1"].toInt();
        int x2_s = det["x2"].toInt();
        int y2_s = det["y2"].toInt();

        int x1_o = static_cast<int>(x1_s * scaleBackX);
        int y1_o = static_cast<int>(y1_s * scaleBackY);
        int x2_o = static_cast<int>(x2_s * scaleBackX);
        int y2_o = static_cast<int>(y2_s * scaleBackY);

        x1_o = std::max(0, x1_o);
        y1_o = std::max(0, y1_o);
        x2_o = std::min(mat.cols - 1, x2_o);
        y2_o = std::min(mat.rows - 1, y2_o);

        cv::rectangle(mat, cv::Rect(x1_o, y1_o, x2_o - x1_o, y2_o - y1_o),
                      cv::Scalar(0, 255, 0), 2);

        QString label = QString("%1 (%2)").arg(cls).arg(conf, 0, 'f', 2);
        std::string text = label.toStdString();

        cv::Point org(x1_o, y1_o - 5);
        cv::putText(mat, text, org, cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    cv::Scalar(0, 255, 0), 2);
    }

    return CvMatToQImage(mat);
}
