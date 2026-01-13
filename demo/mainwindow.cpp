#include "mainwindow.h"
#include <QFileInfo>
#include <QDateTime>
#include <QStandardPaths>
#include <QApplication>
#include <QGroupBox>
#include <QFile>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , workerThread(nullptr)
    , worker(nullptr)
    , compressionRatio(0.0)
    , originalSize(0)
    , compressedSize(0)
{
    setupUI();
    setupConnections();
    setupWorkerThread();

    setWindowTitle("Zstd BMP 压缩测试工具");
    resize(1200, 800);

    setUiEnabled(true);
}

MainWindow::~MainWindow()
{
    // 清理工作线程
    if (workerThread) {
        workerThread->quit();
        workerThread->wait(1000); // 等待1秒
        if (workerThread->isRunning()) {
            workerThread->terminate();
        }
        delete workerThread;
    }
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 创建图像显示区域
    QHBoxLayout *imageLayout = new QHBoxLayout();

    originalImageWidget = new ImageWidget("原始图像", this);
    compressedImageWidget = new ImageWidget("解压后图像", this);

    imageLayout->addWidget(originalImageWidget);
    imageLayout->addWidget(compressedImageWidget);

    // 创建信息显示区域
    QHBoxLayout *infoLayout = new QHBoxLayout();

    originalInfoLabel = new QLabel("原始图像信息：", this);
    compressedInfoLabel = new QLabel("压缩后信息：", this);
    ratioLabel = new QLabel("压缩比：0%", this);

    originalInfoLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 8px; border: 1px solid #ccc; border-radius: 4px; }");
    compressedInfoLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 8px; border: 1px solid #ccc; border-radius: 4px; }");
    ratioLabel->setStyleSheet("QLabel { background-color: #e8f5e8; padding: 8px; border: 1px solid #4CAF50; border-radius: 4px; color: #2E7D32; font-weight: bold; }");

    originalInfoLabel->setMinimumHeight(60);
    compressedInfoLabel->setMinimumHeight(60);
    ratioLabel->setMinimumHeight(60);

    infoLayout->addWidget(originalInfoLabel);
    infoLayout->addWidget(compressedInfoLabel);
    infoLayout->addWidget(ratioLabel);

    // 创建控制按钮区域
    QGroupBox *controlGroup = new QGroupBox("操作控制", this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);

    openButton = new QPushButton("打开图像", this);
    compressButton = new QPushButton("压缩", this);
    decompressButton = new QPushButton("解压", this);
    saveCompressedButton = new QPushButton("保存压缩文件", this);
    saveDecompressedButton = new QPushButton("保存解压图像", this);

    // 设置按钮样式
    QString buttonStyle = "QPushButton { padding: 8px 16px; font-weight: bold; }";
    openButton->setStyleSheet(buttonStyle);
    compressButton->setStyleSheet(buttonStyle);
    decompressButton->setStyleSheet(buttonStyle);
    saveCompressedButton->setStyleSheet(buttonStyle);
    saveDecompressedButton->setStyleSheet(buttonStyle);

    compressButton->setEnabled(false);
    decompressButton->setEnabled(false);
    saveCompressedButton->setEnabled(false);
    saveDecompressedButton->setEnabled(false);

    controlLayout->addWidget(openButton);
    controlLayout->addWidget(compressButton);
    controlLayout->addWidget(decompressButton);
    controlLayout->addWidget(saveCompressedButton);
    controlLayout->addWidget(saveDecompressedButton);
    controlLayout->addStretch();

    // 创建参数设置区域
    QGroupBox *paramGroup = new QGroupBox("压缩参数", this);
    QHBoxLayout *paramLayout = new QHBoxLayout(paramGroup);

    QLabel *formatLabel = new QLabel("输出格式:", this);
    formatCombo = new QComboBox(this);
    formatCombo->addItems({"BMP", "PNG", "JPEG"});
    formatCombo->setCurrentIndex(0);

    QLabel *levelLabel = new QLabel("压缩级别:", this);
    levelSpin = new QSpinBox(this);
    levelSpin->setRange(1, 22);
    levelSpin->setValue(3);
    levelSpin->setToolTip("1=最快, 22=最好压缩率");

    paramLayout->addWidget(formatLabel);
    paramLayout->addWidget(formatCombo);
    paramLayout->addSpacing(20);
    paramLayout->addWidget(levelLabel);
    paramLayout->addWidget(levelSpin);
    paramLayout->addStretch();

    // 进度条
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setRange(0, 100);
    progressBar->setTextVisible(true);

    // 添加到主布局
    mainLayout->addLayout(imageLayout, 1);
    mainLayout->addLayout(infoLayout);
    mainLayout->addWidget(controlGroup);
    mainLayout->addWidget(paramGroup);
    mainLayout->addWidget(progressBar);
    mainLayout->addStretch();

    // 状态栏
    statusBar()->showMessage("就绪");
}

void MainWindow::setupWorkerThread()
{
    // 创建工作线程
    workerThread = new QThread(this);
    worker = new CompressWorker();

    // 将worker移动到工作线程
    worker->moveToThread(workerThread);

    // 连接工作线程信号
    connect(workerThread, &QThread::started, worker, []() {
        qDebug() << QStringLiteral("Worker thread started");
    });

    connect(workerThread, &QThread::finished, worker, &CompressWorker::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);

    // 连接worker信号到主线程槽函数
    connect(worker, &CompressWorker::compressionFinished,
            this, &MainWindow::onCompressionFinished,Qt::QueuedConnection);
    connect(worker, &CompressWorker::decompressionFinished,
            this, &MainWindow::onDecompressionFinished, Qt::QueuedConnection);
    connect(worker, &CompressWorker::errorOccurred,
            this, &MainWindow::onWorkerError, Qt::QueuedConnection);
    connect(worker, &CompressWorker::progressChanged,
            this, &MainWindow::onProgressChanged, Qt::QueuedConnection);

    // 启动工作线程
    workerThread->start();
}

void MainWindow::setupConnections()
{
    connect(openButton, &QPushButton::clicked, this, &MainWindow::onOpenImage);
    connect(compressButton, &QPushButton::clicked, this, &MainWindow::onCompressImage);
    connect(decompressButton, &QPushButton::clicked, this, &MainWindow::onDecompressImage);
    connect(saveCompressedButton, &QPushButton::clicked, this, &MainWindow::onSaveCompressed);
    connect(saveDecompressedButton, &QPushButton::clicked, this, &MainWindow::onSaveDecompressed);
    connect(levelSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onCompressionLevelChanged);
    connect(formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFormatChanged);
}

void MainWindow::setUiEnabled(const bool enabled) const
{
    openButton->setEnabled(enabled);
    compressButton->setEnabled(enabled && !currentImage.isNull());
    decompressButton->setEnabled(enabled);
    saveCompressedButton->setEnabled(enabled && !compressedData.isEmpty());
    saveDecompressedButton->setEnabled(enabled && !decompressedImage.isNull());
    formatCombo->setEnabled(enabled);
    levelSpin->setEnabled(enabled);
}

void MainWindow::onOpenImage()
{
    QString filter = "图像文件 (*.bmp *.png *.jpg *.jpeg *.tiff *.tif);;所有文件 (*.*)";
    QString fileName = QFileDialog::getOpenFileName(this, "打开图像文件",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), filter);

    if (fileName.isEmpty()) return;

    if (currentImage.load(fileName)) {
        originalImageWidget->setImage(currentImage);
        currentFile = fileName;
        originalFormat = QFileInfo(fileName).suffix().toUpper();

        QFileInfo fileInfo(fileName);
        originalSize = fileInfo.size();

        compressButton->setEnabled(true);
        decompressButton->setEnabled(true);
        updateImageInfo();
        updateStatus(QString("已加载图像: %1").arg(fileName));
    } else {
        QMessageBox::warning(this, "错误", "无法加载图像文件");
    }
}

void MainWindow::onCompressImage()
{
    if (currentImage.isNull()) return;

    // 禁用UI防止重复操作
    setUiEnabled(false);
    progressBar->setVisible(true);
    progressBar->setValue(0);

    updateStatus("开始压缩图像...");

    // 在工作线程中执行压缩
    QMetaObject::invokeMethod(worker, "compressImage",
        Qt::QueuedConnection,
        Q_ARG(QImage, currentImage),
        Q_ARG(int, levelSpin->value()),
        Q_ARG(int, formatCombo->currentIndex()));
}

void MainWindow::onDecompressImage()
{
    QString filter = "压缩文件 (*.zstd *.zst);;所有文件 (*.*)";
    QString fileName = QFileDialog::getOpenFileName(this, "打开压缩文件",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), filter);

    if (fileName.isEmpty()) return;

    // 禁用UI防止重复操作
    setUiEnabled(false);
    progressBar->setVisible(true);
    progressBar->setValue(0);

    updateStatus("开始解压文件...");

    // 在工作线程中执行解压
    QMetaObject::invokeMethod(worker, "decompressFile",
        Qt::QueuedConnection,
        Q_ARG(QString, fileName));
}

void MainWindow::onCompressionFinished(const QByteArray& compressedData,
                                     size_t originalSize,
                                     size_t compressedSize,
                                     double ratio)
{
    this->compressedData = compressedData;
    this->originalSize = originalSize;
    this->compressedSize = compressedSize;
    this->compressionRatio = ratio;

    // 更新UI
    compressedInfoLabel->setText(QString("压缩后: %1 bytes\n压缩比: %2%")
        .arg(compressedSize)
        .arg(ratio * 100, 0, 'f', 2));

    ratioLabel->setText(QString("压缩比: %1%").arg(ratio * 100, 0, 'f', 2));

    updateStatus(QString("压缩完成: 原始大小 %1 bytes, 压缩后 %2 bytes, 压缩比 %3%")
        .arg(originalSize)
        .arg(compressedSize)
        .arg(ratio * 100, 0, 'f', 2));

    // 重新启用UI
    progressBar->setVisible(false);
    setUiEnabled(true);
}

void MainWindow::onDecompressionFinished(const QImage& image,
                                       size_t compressedSize,
                                       size_t decompressedSize)
{
    decompressedImage = image;

    if (!decompressedImage.isNull()) {
        compressedImageWidget->setImage(decompressedImage);

        this->compressedSize = compressedSize;
        this->originalSize = decompressedSize;
        this->compressionRatio = static_cast<double>(compressedSize) / decompressedSize;

        compressedInfoLabel->setText(QString("解压结果: %1 x %2\n文件大小: %3 bytes")
            .arg(decompressedImage.width())
            .arg(decompressedImage.height())
            .arg(decompressedSize));

        ratioLabel->setText(QString("压缩比: %1%").arg(compressionRatio * 100, 0, 'f', 2));

        updateStatus(QString("解压完成: 图像大小 %1 x %2")
            .arg(decompressedImage.width())
            .arg(decompressedImage.height()));
    }

    // 重新启用UI
    progressBar->setVisible(false);
    setUiEnabled(true);
}

void MainWindow::onWorkerError(const QString& errorMessage)
{
    QMessageBox::warning(this, "操作失败", errorMessage);

    // 重新启用UI
    progressBar->setVisible(false);
    setUiEnabled(true);

    updateStatus("操作失败: " + errorMessage);
}

void MainWindow::onProgressChanged(int progress)
{
    progressBar->setValue(progress);
    updateStatus(QString("处理中... %1%").arg(progress));
}

void MainWindow::onSaveCompressed()
{
    if (compressedData.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有压缩数据可保存");
        return;
    }

    QString filter = "Zstd压缩文件 (*.zstd);";
    QString defaultName = QFileInfo(currentFile).completeBaseName() + ".zstd";
    QString fileName = QFileDialog::getSaveFileName(this, "保存压缩文件",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + defaultName, filter);

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(compressedData);
        file.close();
        updateStatus(QString("压缩文件已保存: %1").arg(fileName));
        QMessageBox::information(this, "成功", "压缩文件保存成功");
    } else {
        QMessageBox::warning(this, "错误", "无法保存文件");
    }
}

void MainWindow::onSaveDecompressed()
{
    if (decompressedImage.isNull()) {
        QMessageBox::warning(this, "错误", "没有解压图像可保存");
        return;
    }

    QString filter = "BMP文件 (*.bmp);;PNG文件 (*.png);;JPEG文件 (*.jpg)";
    QString defaultName = QFileInfo(currentFile).completeBaseName() + "_decompressed.bmp";
    QString fileName = QFileDialog::getSaveFileName(this, "保存解压图像",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/" + defaultName, filter);

    if (fileName.isEmpty()) return;

    QString format = QFileInfo(fileName).suffix().toUpper();
    if (format == "JPG") format = "JPEG";

    if (decompressedImage.save(fileName, format.toLatin1())) {
        updateStatus(QString("解压图像已保存: %1").arg(fileName));
        QMessageBox::information(this, "成功", "解压图像保存成功");
    } else {
        QMessageBox::warning(this, "错误", "无法保存图像");
    }
}

void MainWindow::onCompressionLevelChanged(int level)
{
    updateStatus(QString("压缩级别已设置为: %1").arg(level));
}

void MainWindow::onFormatChanged(int index)
{
    QString format = formatCombo->itemText(index);
    updateStatus(QString("输出格式已设置为: %1").arg(format));
}

void MainWindow::updateImageInfo()
{
    if (!currentImage.isNull()) {
        QString info = QString("原始图像: %1\n"
                              "尺寸: %2 x %3\n"
                              "大小: %4 bytes\n"
                              "格式: %5")
            .arg(QFileInfo(currentFile).fileName())
            .arg(currentImage.width())
            .arg(currentImage.height())
            .arg(originalSize)
            .arg(originalFormat);

        originalInfoLabel->setText(info);
    }
}

void MainWindow::updateStatus(const QString& message)
{
    statusBar()->showMessage(QString("[%1] %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(message));

    // 强制更新UI
    QApplication::processEvents();
}