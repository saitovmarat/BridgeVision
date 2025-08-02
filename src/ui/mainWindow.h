// mainWindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "videoPlayer.h"
#include <QMainWindow>
#include <QImage>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_fileSelectionButton_clicked();
    void updateFrame(const QImage &image);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::MainWindow *ui;
    std::unique_ptr<VideoPlayer> player;
    QImage m_lastFrame;  // Для перерисовки при resize
};

#endif // MAINWINDOW_H
