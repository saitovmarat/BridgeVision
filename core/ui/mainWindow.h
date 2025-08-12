#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "iLink.h"
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
    explicit MainWindow(ILink* link, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_fileSelectionButton_clicked();
    void updateFrame(const QImage &image);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    std::unique_ptr<Ui::MainWindow> ui;
    ILink* m_link;
    QImage m_lastFrame;
};

#endif // MAINWINDOW_H
