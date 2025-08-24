#ifndef IMAGECONVERTER_H
#define IMAGECONVERTER_H

#include <QImage>
#include <opencv2/core.hpp>

cv::Mat qImageToCvMat(const QImage &image);
QImage cvMatToQImage(const cv::Mat &mat);

#endif // IMAGECONVERTER_H
