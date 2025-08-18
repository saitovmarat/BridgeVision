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
    ~MainWindow() override;

private slots:
    void onFileSelectionButtonClicked();
    void updateFrame(const QImage &image);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    std::unique_ptr<Ui::MainWindow> ui;
    ILink* m_link;
    QImage m_last_frame;

    Q_DISABLE_COPY_MOVE(MainWindow)
};

#endif // MAINWINDOW_H
