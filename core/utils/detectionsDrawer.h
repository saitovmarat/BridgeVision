#ifndef DETECTIONSDRAWER_H
#define DETECTIONSDRAWER_H

#include <QImage>
#include <QJsonArray>
#include <QSize>


static constexpr int COLOR_MAX = 255;
static constexpr double FONT_SCALE = 0.6;
static constexpr int LINE_THICKNESS = 2;

/**
 * @brief Рисует bounding box'ы и метки на изображении на основе детекций.
 * @param image Исходное изображение.
 * @param detections Массив детекций в формате JSON: {class, confidence, x1, y1, x2, y2}
 * @param sentSize Размер, в котором изображение было отправлено на обработку (для масштабирования).
 * @return QImage с нарисованными детекциями.
 */
QImage drawDetections(const QImage &image, const QJsonArray &detections, const QSize &sent_size);

#endif // DETECTIONSDRAWER_H
