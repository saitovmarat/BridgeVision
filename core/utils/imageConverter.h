#ifndef IMAGECONVERTER_H
#define IMAGECONVERTER_H

#include <QImage>
#include <opencv2/core.hpp>

/**
 * @brief Конвертирует QImage в cv::Mat.
 * Поддерживает Format_RGB888, RGB32, ARGB32.
 * @param image Входное изображение.
 * @return cv::Mat (в формате BGR).
 */
cv::Mat qImageToCvMat(const QImage &image);

/**
 * @brief Конвертирует cv::Mat в QImage.
 * Поддерживает CV_8UC3 (BGR) и CV_8UC4 (BGRA).
 * @param mat Входная матрица OpenCV.
 * @return QImage (в формате RGB888).
 */
QImage cvMatToQImage(const cv::Mat &mat);

#endif // IMAGECONVERTER_H
