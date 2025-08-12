#include "mainWindow.h"
#include "ui_mainWindow.h"

#include <QFileDialog>


MainWindow::MainWindow(ILink* link, QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , m_link(link)
{
    ui->setupUi(this);
    setWindowTitle("Bridge Vision");
    setWindowIcon(QIcon(":/app_icon.png"));

    connect(m_link, &ILink::frameReady, this, &MainWindow::updateFrame, Qt::QueuedConnection);
    connect(m_link, &ILink::errorOccurred, this, [this](const QString &error) {
        ui->statusBar->showMessage("Ошибка: " + error);
        qDebug() << "Server error:" << error;
    });
    connect(m_link, &ILink::playbackFinished, this, [this]() {
        ui->statusBar->showMessage("Видео завершено");
    });

    ui->cameraLabel->setText("Видео не загружено");
    ui->cameraLabel->setAlignment(Qt::AlignCenter);
}

MainWindow::~MainWindow() = default;

void MainWindow::on_fileSelectionButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Выберите видео-файл",
        QString(PROJECT_SOURCE_DIR),
        "Видео файлы (*.mp4);;Все файлы (*)"
    );

    if (!filePath.isEmpty()) {
        qDebug() << "Выбран MP4-файл:" << filePath;

        if (!m_link->loadVideo(filePath)) {
            ui->statusBar->showMessage("Ошибка загрузки видео");
        } else {
            m_link->play();

            ui->statusBar->showMessage("Воспроизведение: " + filePath);
        }
    }
}

void MainWindow::updateFrame(const QImage &image)
{
    if (image.isNull()) {
        return;
    }

    m_lastFrame = image;

    QPixmap pixmap = QPixmap::fromImage(image).scaled(
        ui->cameraLabel->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    ui->cameraLabel->setPixmap(pixmap);
    ui->cameraLabel->repaint();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (!m_lastFrame.isNull()) {
        updateFrame(m_lastFrame);
    }
    QMainWindow::resizeEvent(event);
}

