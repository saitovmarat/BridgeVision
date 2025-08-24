#ifndef OVERLAYRENDERER_H
#define OVERLAYRENDERER_H

#include <QImage>
#include <QJsonArray>
#include <QSize>


static constexpr int COLOR_MAX = 255;
static constexpr double FONT_SCALE = 0.6;
static constexpr int LINE_THICKNESS = 2;
static constexpr int K_ARCH_CENTER_POINT_SIZE = 6;
static constexpr int K_TEXT_OFFSET = 10;

QImage drawDetections(const QImage &image, const QJsonArray &detections, const QSize &sent_size);
QImage drawArchCenter(const QImage& original_image, const QJsonObject& arch_center, const QSize& sent_size);

#endif // OVERLAYRENDERER_H
