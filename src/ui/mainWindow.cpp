#include "mainWindow.h"
#include "ui_mainWindow.h"


#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , player(new VideoPlayer(this))
{
    ui->setupUi(this);

    connect(player, &VideoPlayer::frameReady, this, &MainWindow::updateFrame, Qt::QueuedConnection);
    connect(player, &VideoPlayer::errorOccurred, this, [this](const QString &error) {
        ui->statusBar->showMessage("Ошибка: " + error);
        qDebug() << "VideoPlayer error:" << error;
    });
    connect(player, &VideoPlayer::playbackFinished, this, [this]() {
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

        if (!player->loadVideo(filePath)) {
            ui->statusBar->showMessage("Ошибка загрузки видео");
        } else {
            player->play();
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

