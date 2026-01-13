#include "compressworker.h"
#include <QThread>
#include <QDebug>
#include <QFileInfo>
#include <QFile>

CompressWorker::CompressWorker(QObject *parent)
    : QObject(parent)
    , m_compressor(new zstd_compressor::ImageCompressor())
{
    qDebug() << "CompressWorker created in thread:" << QThread::currentThreadId();
}

zstd_compressor::ImageFormat CompressWorker::convertFormat(int format) const
{
    static const QHash<int, zstd_compressor::ImageFormat> formatMap = {
        {0, zstd_compressor::ImageFormat::FORMAT_BMP},
        {1, zstd_compressor::ImageFormat::FORMAT_PNG},
        {2, zstd_compressor::ImageFormat::FORMAT_JPEG}
    };

    return formatMap.value(format, zstd_compressor::ImageFormat::FORMAT_BMP);
}

void CompressWorker::handleCompressionError(const QString& context, const QString& error)
{
    QString errorMessage = context;
    if (!error.isEmpty()) {
        errorMessage += ": " + error;
    }

    qWarning() << "CompressWorker Error:" << errorMessage;
    emit errorOccurred(errorMessage);
}

void CompressWorker::updateProgress(int value)
{
    // 限制进度更新频率，避免过多信号发射
    static int lastProgress = -1;
    if (value != lastProgress && (value % 10 == 0 || value == 100 || value == 0)) {
        emit progressChanged(value);
        lastProgress = value;
    }
}

void CompressWorker::compressImage(const QImage& image, int level, int format)
{
    qDebug() << "CompressWorker: Starting compression in thread:" << QThread::currentThreadId();

    if (image.isNull()) {
        handleCompressionError("图像为空");
        return;
    }

    if (level < 0 || level > 22) {
        handleCompressionError("压缩级别无效");
        return;
    }

    try {
        // 设置压缩参数
        m_compressor->setCompressionLevel(level);
        m_compressor->setImageFormat(convertFormat(format));

        updateProgress(10);

        // 加载图像
        if (!m_compressor->loadImage(image)) {
            handleCompressionError("无法加载图像");
            return;
        }

        updateProgress(30);

        // 执行压缩
        auto result = m_compressor->compress();

        if (!result.success()) {
            handleCompressionError("压缩失败");
            return;
        }

        updateProgress(70);

        // 获取压缩数据
        auto compressedData = m_compressor->getCompressedData();
        QByteArray compressedByteArray(reinterpret_cast<const char*>(compressedData.data()),
                                     static_cast<int>(compressedData.size()));

        updateProgress(90);

        // 验证压缩结果
        if (compressedByteArray.isEmpty()) {
            handleCompressionError("压缩数据为空");
            return;
        }

        // 发送完成信号
        emit compressionFinished(compressedByteArray,
                               result.original_size,
                               result.compressed_size,
                               result.compression_ratio);

        updateProgress(100);

        qDebug() << "CompressWorker: Compression completed successfully. Ratio:"
                 << result.compression_ratio;

    } catch (const std::exception& e) {
        handleCompressionError("压缩异常", e.what());
    } catch (...) {
        handleCompressionError("未知压缩异常");
    }
}

void CompressWorker::decompressData(const QByteArray& compressedData)
{
    qDebug() << "CompressWorker: Starting decompression in thread:" << QThread::currentThreadId();

    if (compressedData.isEmpty()) {
        handleCompressionError("压缩数据为空");
        return;
    }

    try {
        updateProgress(10);

        // 避免不必要的拷贝，直接使用数据指针
        std::vector<unsigned char> compressedVec(compressedData.begin(), compressedData.end());

        updateProgress(30);

        // 执行解压 - 正确处理 CompressionResult 返回值
        auto result = m_compressor->decompress(compressedVec);

        if (!result.success()) {
            handleCompressionError(QString("解压失败: %1").arg(result.error_message.c_str()));
            return;
        }

        updateProgress(70);

        // 获取解压后的图像
        QImage decompressedImage = m_compressor->getQImage();

        if (decompressedImage.isNull()) {
            handleCompressionError("解压后的图像无效");
            return;
        }

        updateProgress(90);

        auto decompressedData = m_compressor->getDecompressedData();

        // 发送完成信号
        emit decompressionFinished(decompressedImage,
                                  compressedData.size(),
                                  decompressedData.size());

        updateProgress(100);

        qDebug() << "CompressWorker: Decompression completed successfully. Size:"
                 << decompressedImage.size();

    } catch (const std::exception& e) {
        handleCompressionError("解压异常", e.what());
    } catch (...) {
        handleCompressionError("未知解压异常");
    }
}

void CompressWorker::decompressFile(const QString& filename)
{
    qDebug() << "CompressWorker: Starting file decompression in thread:" << QThread::currentThreadId();

    if (filename.isEmpty()) {
        handleCompressionError("文件名为空");
        return;
    }

    QFileInfo fileInfo(filename);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        handleCompressionError("文件不存在或不是有效文件");
        return;
    }

    if (fileInfo.size() == 0) {
        handleCompressionError("文件为空");
        return;
    }

    try {
        updateProgress(10);

        // 从文件解压 - 正确处理 CompressionResult 返回值
        auto result = m_compressor->decompressFromFile(filename);

        if (!result.success()) {
            handleCompressionError(QString("文件解压失败: %1").arg(result.error_message.c_str()));
            return;
        }

        updateProgress(60);

        // 获取解压后的图像
        QImage decompressedImage = m_compressor->getQImage();

        if (decompressedImage.isNull()) {
            handleCompressionError("解压后的图像无效");
            return;
        }

        updateProgress(80);

        auto decompressedData = m_compressor->getDecompressedData();
        size_t compressedSize = static_cast<size_t>(fileInfo.size());

        // 验证解压结果
        if (decompressedData.empty()) {
            handleCompressionError("解压数据为空");
            return;
        }

        updateProgress(90);

        // 发送完成信号
        emit decompressionFinished(decompressedImage,
                                  compressedSize,
                                  decompressedData.size());

        updateProgress(100);

        qDebug() << "CompressWorker: File decompression completed successfully. Size:"
                 << decompressedImage.size();

    } catch (const std::exception& e) {
        handleCompressionError("文件解压异常", e.what());
    } catch (...) {
        handleCompressionError("未知文件解压异常");
    }
}