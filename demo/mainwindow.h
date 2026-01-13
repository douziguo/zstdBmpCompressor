#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include "imagewidget.h"
#include "compressworker.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenImage();
    void onCompressImage();
    void onDecompressImage();
    void onSaveCompressed();
    void onSaveDecompressed();
    void onCompressionLevelChanged(int level);
    void onFormatChanged(int index);

    // 工作线程信号处理
    void onCompressionFinished(const QByteArray& compressedData,
                             size_t originalSize,
                             size_t compressedSize,
                             double ratio);
    void onDecompressionFinished(const QImage& image,
                               size_t compressedSize,
                               size_t decompressedSize);
    void onWorkerError(const QString& errorMessage);
    void onProgressChanged(int progress);

private:
    void setupUI();
    void setupConnections();
    void setupWorkerThread();
    void updateImageInfo();
    void updateStatus(const QString& message);
    void setUiEnabled(const bool enabled) const;

    // UI组件
    ImageWidget *originalImageWidget;
    ImageWidget *compressedImageWidget;
    QLabel *originalInfoLabel;
    QLabel *compressedInfoLabel;
    QLabel *ratioLabel;

    QPushButton *openButton;
    QPushButton *compressButton;
    QPushButton *decompressButton;
    QPushButton *saveCompressedButton;
    QPushButton *saveDecompressedButton;

    QComboBox *formatCombo;
    QSpinBox *levelSpin;
    QProgressBar *progressBar;

    // 工作线程
    QThread *workerThread;
    CompressWorker *worker;

    // 数据
    QImage currentImage;
    QImage decompressedImage;
    QByteArray compressedData;
    QString currentFile;
    QString originalFormat;

    // 压缩结果
    double compressionRatio;
    size_t originalSize;
    size_t compressedSize;
};

#endif // MAINWINDOW_H