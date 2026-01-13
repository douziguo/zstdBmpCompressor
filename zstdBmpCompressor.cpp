#include "zstdBmpCompressor.h"
#include <zstd.h>
#include <fstream>
#include <filesystem>
#include <QBuffer>
#include <QImageReader>
#include <algorithm>

namespace zstd_compressor {

// ZstdContext 析构函数
ImageCompressor::ZstdContext::~ZstdContext() {
    if (cctx) ZSTD_freeCCtx(cctx);
    if (dctx) ZSTD_freeDCtx(dctx);
}

ImageCompressor::ImageCompressor(int level)
    : m_level(std::clamp(level, 1, 22))
    , m_num_threads(4)
    , m_format(ImageFormat::FORMAT_BMP)
    , m_ctx(std::make_unique<ZstdContext>()) {
}

ImageCompressor::~ImageCompressor() = default;

void ImageCompressor::setCompressionLevel(int level) {
    m_level = std::clamp(level, 1, 22);
}

void ImageCompressor::setImageFormat(ImageFormat format) {
    m_format = format;
}

void ImageCompressor::setNumThreads(int num_threads) {
    m_num_threads = (num_threads > 0) ? num_threads : 1;
}

bool ImageCompressor::loadImage(const std::string& filename) {
    clearResults();
    return loadImageFile(filename);
}

bool ImageCompressor::loadImage(const QString& filename) {
    return loadImage(filename.toStdString());
}

bool ImageCompressor::loadImage(const QImage& image) {
    clearResults();
    m_qImage = image;
    return convertToImageData(image);
}

bool ImageCompressor::loadImage(const cv::Mat& image) {
    clearResults();
    m_cvMat = image;
    return convertToImageData(image);
}

bool ImageCompressor::loadImage(std::vector<unsigned char> data) {
    clearResults();
    m_originalData = std::move(data);
    return !m_originalData.empty();
}

bool ImageCompressor::loadImageFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    const auto size = file.tellg();
    if (size <= 0) return false;

    file.seekg(0, std::ios::beg);
    m_originalData.resize(static_cast<size_t>(size));

    if (!file.read(reinterpret_cast<char*>(m_originalData.data()), size)) {
        m_originalData.clear();
        return false;
    }

    // 可选：验证图像数据有效性
    try {
        m_qImage.load(QString::fromStdString(filename));
        m_cvMat = cv::imread(filename);
    } catch (...) {
        // 忽略图像解码错误，原始数据仍然有效
    }

    return true;
}

bool ImageCompressor::convertToImageData(const QImage& image) {
    if (image.isNull()) return false;

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);

    const char* format = "BMP";
    switch (m_format) {
        case ImageFormat::FORMAT_PNG: format = "PNG"; break;
        case ImageFormat::FORMAT_JPEG: format = "JPEG"; break;
        default: format = "BMP"; break;
    }

    if (!image.save(&buffer, format)) {
        return false;
    }

    m_originalData.assign(byteArray.begin(), byteArray.end());
    return true;
}

bool ImageCompressor::convertToImageData(const cv::Mat& image) {
    if (image.empty()) return false;

    std::vector<unsigned char> buffer;
    const char* extension = ".bmp";
    switch (m_format) {
        case ImageFormat::FORMAT_PNG: extension = ".png"; break;
        case ImageFormat::FORMAT_JPEG: extension = ".jpg"; break;
        default: extension = ".bmp"; break;
    }

    if (!cv::imencode(extension, image, buffer)) {
        return false;
    }

    m_originalData = std::move(buffer);
    return true;
}

CompressionResult ImageCompressor::compress() {
    if (m_originalData.empty()) {
        return CompressionResult(CompressResult::ERROR_EMPTY_DATA, "No image data loaded");
    }
    return compressInternal();
}

CompressionResult ImageCompressor::compressImage(const QImage& image) {
    if (!loadImage(image)) {
        return CompressionResult(CompressResult::ERROR_EMPTY_DATA, "Failed to load QImage");
    }
    return compress();
}

CompressionResult ImageCompressor::compressImage(const cv::Mat& image) {
    if (!loadImage(image)) {
        return CompressionResult(CompressResult::ERROR_EMPTY_DATA, "Failed to load cv::Mat");
    }
    return compress();
}

CompressionResult ImageCompressor::compressData(const std::vector<unsigned char>& data) {
    if (!loadImage(data)) {
        return CompressionResult(CompressResult::ERROR_EMPTY_DATA, "Input data is empty");
    }
    return compress();
}

CompressionResult ImageCompressor::compressInternal() {
    auto& ctx = getContext();
    if (!ctx.cctx) {
        return CompressionResult(CompressResult::ERROR_COMPRESS_FAILED, "Failed to create compression context");
    }

    const size_t maxSize = ZSTD_compressBound(m_originalData.size());
    m_compressedData.resize(maxSize);

    ZSTD_CCtx_setParameter(ctx.cctx, ZSTD_c_compressionLevel, m_level);
    ZSTD_CCtx_setParameter(ctx.cctx, ZSTD_c_nbWorkers, m_num_threads);

    const size_t compressedSize = ZSTD_compress2(ctx.cctx,
        m_compressedData.data(), maxSize,
        m_originalData.data(), m_originalData.size());

    if (ZSTD_isError(compressedSize)) {
        m_compressedData.clear();
        return CompressionResult(CompressResult::ERROR_COMPRESS_FAILED,
                               ZSTD_getErrorName(compressedSize));
    }

    m_compressedData.resize(compressedSize);

    CompressionResult result;
    result.original_size = m_originalData.size();
    result.compressed_size = compressedSize;
    result.compression_ratio = static_cast<double>(compressedSize) / result.original_size;
    result.result_code = CompressResult::SUCCESS;

    return result;
}

CompressionResult ImageCompressor::decompress(const std::vector<unsigned char>& compressedData) {
    clearResults();
    m_compressedData = compressedData;
    return decompressInternal();
}

CompressionResult ImageCompressor::decompressFromFile(const std::string& filename) {
    clearResults();

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return CompressionResult(CompressResult::ERROR_DECOMPRESS_FAILED, "Cannot open file");
    }

    const auto size = file.tellg();
    if (size <= 0) {
        return CompressionResult(CompressResult::ERROR_DECOMPRESS_FAILED, "File is empty");
    }

    file.seekg(0, std::ios::beg);
    m_compressedData.resize(static_cast<size_t>(size));

    if (!file.read(reinterpret_cast<char*>(m_compressedData.data()), size)) {
        return CompressionResult(CompressResult::ERROR_DECOMPRESS_FAILED, "Failed to read file");
    }

    return decompressInternal();
}

CompressionResult ImageCompressor::decompressFromFile(const QString& filename) {
    return decompressFromFile(filename.toStdString());
}

CompressionResult ImageCompressor::decompressInternal() {
    auto& ctx = getContext();
    if (!ctx.dctx) {
        return CompressionResult(CompressResult::ERROR_DECOMPRESS_FAILED, "Failed to create decompression context");
    }

    if (m_compressedData.empty()) {
        return CompressionResult(CompressResult::ERROR_EMPTY_DATA, "No compressed data");
    }

    const size_t decompressedSize = ZSTD_getFrameContentSize(m_compressedData.data(), m_compressedData.size());
    if (decompressedSize == ZSTD_CONTENTSIZE_ERROR) {
        return CompressionResult(CompressResult::ERROR_DECOMPRESS_FAILED, "Invalid compressed data");
    }
    if (decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        return CompressionResult(CompressResult::ERROR_DECOMPRESS_FAILED, "Unknown content size");
    }

    m_decompressedData.resize(decompressedSize);
    const size_t actualSize = ZSTD_decompressDCtx(ctx.dctx,
        m_decompressedData.data(), decompressedSize,
        m_compressedData.data(), m_compressedData.size());

    if (ZSTD_isError(actualSize)) {
        m_decompressedData.clear();
        return CompressionResult(CompressResult::ERROR_DECOMPRESS_FAILED,
                               ZSTD_getErrorName(actualSize));
    }

    if (actualSize != decompressedSize) {
        m_decompressedData.clear();
        return CompressionResult(CompressResult::ERROR_DECOMPRESS_FAILED, "Decompressed size mismatch");
    }

    CompressionResult result;
    result.original_size = actualSize;
    result.compressed_size = m_compressedData.size();
    result.compression_ratio = static_cast<double>(result.compressed_size) / result.original_size;
    result.result_code = CompressResult::SUCCESS;

    return result;
}

bool ImageCompressor::saveCompressedData(const std::string& filename) const {
    if (m_compressedData.empty()) return false;

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    file.write(reinterpret_cast<const char*>(m_compressedData.data()), m_compressedData.size());
    return file.good();
}

bool ImageCompressor::saveDecompressedImage(const std::string& filename) const {
    if (m_decompressedData.empty()) return false;

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    file.write(reinterpret_cast<const char*>(m_decompressedData.data()), m_decompressedData.size());
    return file.good();
}

QImage ImageCompressor::getQImage() const {
    if (m_qImage.isNull() && !m_decompressedData.empty()) {
        m_qImage.loadFromData(m_decompressedData.data(), static_cast<int>(m_decompressedData.size()));
    }
    return m_qImage;
}

cv::Mat ImageCompressor::getCVMat() const {
    if (m_cvMat.empty() && !m_decompressedData.empty()) {
        try {
            m_cvMat = cv::imdecode(m_decompressedData, cv::IMREAD_UNCHANGED);
        } catch (const cv::Exception& e) {
            // 记录错误但不抛出异常
        }
    }
    return m_cvMat;
}

const std::vector<unsigned char>& ImageCompressor::getCompressedData() const {
    return m_compressedData;
}

const std::vector<unsigned char>& ImageCompressor::getDecompressedData() const {
    return m_decompressedData;
}

CompressionResult ImageCompressor::compressFolder(const std::string& inputFolder,
                                                 const std::string& outputFolder) {
    try {
        if (!std::filesystem::create_directories(outputFolder) &&
            !std::filesystem::exists(outputFolder)) {
            return CompressionResult(CompressResult::ERROR_COMPRESS_FAILED, "Cannot create output directory");
        }

        const auto imageFiles = getImageFiles(inputFolder);
        size_t total_original = 0, total_compressed = 0;
        size_t success_count = 0;

        for (const auto& file : imageFiles) {
            const std::string outputFile = outputFolder + "/" +
                std::filesystem::path(file).stem().string() + ".zstd";

            if (loadImage(file)) {
                auto result = compressInternal();
                if (result.success() && saveCompressedData(outputFile)) {
                    total_original += result.original_size;
                    total_compressed += result.compressed_size;
                    success_count++;
                }
            }
        }

        if (success_count == 0) {
            return CompressionResult(CompressResult::ERROR_COMPRESS_FAILED, "No files processed successfully");
        }

        CompressionResult result;
        result.original_size = total_original;
        result.compressed_size = total_compressed;
        result.compression_ratio = static_cast<double>(total_compressed) / total_original;
        result.result_code = CompressResult::SUCCESS;

        return result;

    } catch (const std::exception& e) {
        return CompressionResult(CompressResult::ERROR_COMPRESS_FAILED, e.what());
    }
}

CompressionResult ImageCompressor::decompressFolder(const std::string& inputFolder,
                                                   const std::string& outputFolder) {
    try {
        if (!std::filesystem::create_directories(outputFolder) &&
            !std::filesystem::exists(outputFolder)) {
            return CompressionResult(CompressResult::ERROR_DECOMPRESS_FAILED, "Cannot create output directory");
        }

        size_t success_count = 0;

        for (const auto& entry : std::filesystem::directory_iterator(inputFolder)) {
            if (entry.is_regular_file() && isCompressedFile(entry.path().string())) {
                const std::string outputFile = outputFolder + "/" +
                    entry.path().stem().string() + ".bmp";

                auto result = decompressFromFile(entry.path().string());
                if (result.success() && saveDecompressedImage(outputFile)) {
                    success_count++;
                }
            }
        }

        if (success_count == 0) {
            return CompressionResult(CompressResult::ERROR_DECOMPRESS_FAILED, "No files processed successfully");
        }

        return CompressionResult(CompressResult::SUCCESS);

    } catch (const std::exception& e) {
        return CompressionResult(CompressResult::ERROR_DECOMPRESS_FAILED, e.what());
    }
}

bool ImageCompressor::isImageFile(const std::string& filename) {
    static const std::vector<std::string> extensions = {
        ".bmp", ".png", ".jpg", ".jpeg", ".tiff", ".tif", ".webp"
    };

    const std::string ext = std::filesystem::path(filename).extension().string();
    return std::find(extensions.begin(), extensions.end(), ext) != extensions.end();
}

bool ImageCompressor::isCompressedFile(const std::string& filename) {
    const std::string ext = std::filesystem::path(filename).extension().string();
    return ext == ".zstd" || ext == ".zst";
}

std::vector<std::string> ImageCompressor::getImageFiles(const std::string& folder) {
    std::vector<std::string> files;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (entry.is_regular_file() && isImageFile(entry.path().string())) {
                files.push_back(entry.path().string());
            }
        }
    } catch (const std::exception&) {
        // 记录错误但不抛出异常
    }
    std::sort(files.begin(), files.end());
    return files;
}

void ImageCompressor::clearResults() {
    m_compressedData.clear();
    m_decompressedData.clear();
    m_qImage = QImage();
    m_cvMat = cv::Mat();
}

ImageCompressor::ZstdContext& ImageCompressor::getContext() {
    if (!m_ctx->cctx) {
        m_ctx->cctx = ZSTD_createCCtx();
    }
    if (!m_ctx->dctx) {
        m_ctx->dctx = ZSTD_createDCtx();
    }
    return *m_ctx;
}

} // namespace zstd_compressor