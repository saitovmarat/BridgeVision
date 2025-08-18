#include "detectionsDrawer.h"
#include "imageConverter.h"

#include <QImage>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QSizeF>
#include <QSize>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>


QImage drawDetections(
    const QImage &original_image,
    const QJsonArray &detections,
    const QSize &sent_size)
{
    if (original_image.isNull() || detections.isEmpty()) {
        return original_image;
    }

    cv::Mat mat = qImageToCvMat(original_image);

    const QSizeF orig_size(original_image.width(), original_image.height());
    QSizeF scaled_size = orig_size;
    scaled_size.scale(sent_size, Qt::KeepAspectRatio);

    const double scale_back_x = orig_size.width() / scaled_size.width();
    const double scale_back_y = orig_size.height() / scaled_size.height();

    for (const auto &val : detections) {
        QJsonObject det = val.toObject();
        const QString cls = det["class"].toString();
        const double conf = det["confidence"].toDouble();
        const int x1_s = det["x1"].toInt();
        const int y1_s = det["y1"].toInt();
        const int x2_s = det["x2"].toInt();
        const int y2_s = det["y2"].toInt();

        int x1_o = static_cast<int>(x1_s * scale_back_x);
        int y1_o = static_cast<int>(y1_s * scale_back_y);
        int x2_o = static_cast<int>(x2_s * scale_back_x);
        int y2_o = static_cast<int>(y2_s * scale_back_y);

        x1_o = std::max(0, x1_o);
        y1_o = std::max(0, y1_o);
        x2_o = std::min(mat.cols - 1, x2_o);
        y2_o = std::min(mat.rows - 1, y2_o);

        cv::rectangle(mat, cv::Rect(x1_o, y1_o, x2_o - x1_o, y2_o - y1_o),
                      cv::Scalar(0, COLOR_MAX, 0), LINE_THICKNESS);

        const QString label = QString("%1 (%2)").arg(cls).arg(conf, 0, 'f', 2);
        const std::string text = label.toStdString();

        const cv::Point org(x1_o, y1_o - 5);
        cv::putText(mat, text, org, cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE,
            cv::Scalar(0, COLOR_MAX, 0), LINE_THICKNESS);
    }

    return cvMatToQImage(mat);
}
