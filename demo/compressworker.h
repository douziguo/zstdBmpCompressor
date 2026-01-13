#ifndef COMPRESSWORKER_H
#define COMPRESSWORKER_H

#include <QObject>
#include <QScopedPointer>
#include "zstdBmpCompressor.h"

class CompressWorker : public QObject
{
    Q_OBJECT

public:
    explicit CompressWorker(QObject *parent = nullptr);
    ~CompressWorker() = default;

public slots:
    void compressImage(const QImage& image, int level, int format);
    void decompressData(const QByteArray& compressedData);
    void decompressFile(const QString& filename);

    signals:
        void compressionFinished(const QByteArray& compressedData,
                               size_t originalSize,
                               size_t compressedSize,
                               double ratio);
    void decompressionFinished(const QImage& image,
                             size_t compressedSize,
                             size_t decompressedSize);
    void errorOccurred(const QString& errorMessage);
    void progressChanged(int progress);

private:
    // 辅助方法
    zstd_compressor::ImageFormat convertFormat(int format) const;
    void handleCompressionError(const QString& context, const QString& error = QString());
    void updateProgress(int value);

    // 使用智能指针管理资源
    QScopedPointer<zstd_compressor::ImageCompressor> m_compressor;
};

#endif // COMPRESSWORKER_H