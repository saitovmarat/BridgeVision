#include "overlayRenderer.h"
#include "imageConverter.h"

#include <QPainter>
#include <QPen>
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

        cv::putText(mat,
            QString::number(conf, 'f', 2).toStdString(),
            cv::Point(x1_o, y1_o),
            cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE,
            cv::Scalar(0, COLOR_MAX, 0), LINE_THICKNESS
        );
    }

    return cvMatToQImage(mat);
}


QImage drawTargetPoint(
    const QImage& original_image,
    const QJsonObject& arch_center,
    const QSize& sent_size)
{
    if (arch_center.isEmpty() || original_image.isNull()) {
        return original_image;
    }

    cv::Mat mat = qImageToCvMat(original_image);

    const QSizeF orig_size(original_image.width(), original_image.height());
    QSizeF scaled_size = orig_size;
    scaled_size.scale(sent_size, Qt::KeepAspectRatio);

    const double scale_back_x = orig_size.width() / scaled_size.width();
    const double scale_back_y = orig_size.height() / scaled_size.height();

    const int x_s = arch_center["x"].toInt();
    const int y_s = arch_center["y"].toInt();

    int x_o = static_cast<int>(x_s * scale_back_x);
    int y_o = static_cast<int>(y_s * scale_back_y);

    x_o = std::max(0, std::min(x_o, mat.cols - 1));
    y_o = std::max(0, std::min(y_o, mat.rows - 1));

    cv::circle(mat, cv::Point(x_o, y_o), K_ARCH_CENTER_POINT_SIZE, cv::Scalar(0, COLOR_MAX, 0), -1);

    const QString label = "Arch";
    const std::string text = label.toStdString();
    const cv::Point org(x_o + K_TEXT_OFFSET, y_o - K_TEXT_OFFSET);
    cv::putText(mat, text, org, cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE, cv::Scalar(0, COLOR_MAX, 0), LINE_THICKNESS);

    return cvMatToQImage(mat);
}