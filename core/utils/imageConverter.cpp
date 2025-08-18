#include "imageConverter.h"
#include <QDebug>
#include <QImage>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <QImage>
#include <QColor>
#include <qglobal.h>


cv::Mat qImageToCvMat(const QImage &image)
{
    if (image.isNull()) {
        return cv::Mat{};
    }

    cv::Mat mat;
    switch (image.format()) {
    case QImage::Format_RGB888:
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, const_cast<uchar*>(image.bits()), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
        break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGR);
        break;
    default:
        qWarning() << "Unsupported QImage format:" << image.format();
        return cv::Mat{};
    }
    return mat;
}

QImage cvMatToQImage(const cv::Mat &mat)
{
    if (mat.empty()) {
        return QImage{};
    }

    cv::Mat rgb;
    switch (mat.type()) {
    case CV_8UC3:
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        break;
    case CV_8UC4:
        cv::cvtColor(mat, rgb, cv::COLOR_BGRA2RGB);
        break;
    default:
        qWarning() << "Unsupported cv::Mat type:" << mat.type();
        return QImage{};
    }

    QImage image(rgb.cols, rgb.rows, QImage::Format_RGB888);
    for (int i = 0; i < rgb.rows; ++i) {
        memcpy(image.scanLine(i), rgb.ptr(i), static_cast<long long>(rgb.cols) * 3);
    }

    return image;
}
