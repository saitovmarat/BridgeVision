#include "mainWindow.h"
#include "ui_mainWindow.h"

#include <QMainWindow>
#include <QFileDialog>
#include <QLabel>
#include <QStatusBar>
#include <QImage>
#include <QPixmap>
#include <QResizeEvent>
#include <QIcon>
#include <QDebug>
#include <QMessageBox>
#include <QMetaObject> 


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
    });
    connect(m_link, &ILink::playbackFinished, this, [this]() {
        ui->statusBar->showMessage("Видео завершено");
    });
    connect(ui->fileSelectionButton, &QPushButton::clicked, this, &MainWindow::onFileSelectionButtonClicked);

    ui->cameraLabel->setText("Видео не загружено");
    ui->cameraLabel->setAlignment(Qt::AlignCenter);
}

MainWindow::~MainWindow() 
{
    m_link->stop();
}

void MainWindow::onFileSelectionButtonClicked()
{
    const QString file_path = QFileDialog::getOpenFileName(
        this,
        "Выберите видео-файл",
        QString(PROJECT_SOURCE_DIR),
        "Видео файлы (*.mp4);;Все файлы (*)"
    );

    if (!file_path.isEmpty()) {
        qDebug() << "Выбран MP4-файл:" << file_path;

        if (!m_link->loadVideo(file_path)) {
            ui->statusBar->showMessage("Ошибка загрузки видео");
        } else {
            m_link->play();

            ui->statusBar->showMessage("Воспроизведение: " + file_path);
        }
    }
}

void MainWindow::updateFrame(const QImage &image)
{
    if (image.isNull()) {
        return;
    }

    m_last_frame = image;

    const QPixmap pixmap = QPixmap::fromImage(image).scaled(
        ui->cameraLabel->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    ui->cameraLabel->setPixmap(pixmap);
    ui->cameraLabel->repaint();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (!m_last_frame.isNull()) {
        updateFrame(m_last_frame);
    }
    QMainWindow::resizeEvent(event);
}

