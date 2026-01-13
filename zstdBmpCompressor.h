#ifndef ZSTDBMPCOMPRESSOR_H
#define ZSTDBMPCOMPRESSOR_H

#include <vector>
#include <string>
#include <memory>
#include <QImage>
#include <zstd.h>
#include <opencv2/opencv.hpp>

#ifdef _WIN32
    #ifdef BMP_COMPRESSOR_EXPORTS
        #define BMP_API __declspec(dllexport)
    #else
        #define BMP_API __declspec(dllimport)
    #endif
#else
    #define BMP_API __attribute__((visibility("default")))
#endif

namespace zstd_compressor {

    enum class ImageFormat { FORMAT_BMP, FORMAT_PNG, FORMAT_JPEG };
    enum class CompressResult { SUCCESS, ERROR_EMPTY_DATA, ERROR_COMPRESS_FAILED, ERROR_DECOMPRESS_FAILED };

    struct CompressionResult {
        size_t original_size = 0;
        size_t compressed_size = 0;
        double compression_ratio = 0.0;
        CompressResult result_code = CompressResult::ERROR_EMPTY_DATA;
        std::string error_message;

        CompressionResult() = default;
        CompressionResult(CompressResult code, const std::string& msg = "")
            : result_code(code), error_message(msg) {}

        bool success() const { return result_code == CompressResult::SUCCESS; }
    };

    class BMP_API ImageCompressor {
    public:
        explicit ImageCompressor(int level = 3);
        ~ImageCompressor();

        // 禁用拷贝和移动
        ImageCompressor(const ImageCompressor&) = delete;
        ImageCompressor& operator=(const ImageCompressor&) = delete;
        ImageCompressor(ImageCompressor&&) = delete;
        ImageCompressor& operator=(ImageCompressor&&) = delete;

        // 设置参数
        void setCompressionLevel(int level);
        void setImageFormat(ImageFormat format);
        void setNumThreads(int num_threads);

        // 加载图像
        bool loadImage(const std::string& filename);
        bool loadImage(const QString& filename);
        bool loadImage(const QImage& image);
        bool loadImage(const cv::Mat& image);
        bool loadImage(std::vector<unsigned char> data); // 移动语义

        // 压缩操作
        CompressionResult compress();
        CompressionResult compressImage(const QImage& image);
        CompressionResult compressImage(const cv::Mat& image);
        CompressionResult compressData(const std::vector<unsigned char>& data);

        // 解压操作
        CompressionResult decompress(const std::vector<unsigned char>& compressedData);
        CompressionResult decompressFromFile(const std::string& filename);
        CompressionResult decompressFromFile(const QString& filename);

        // 保存结果
        bool saveCompressedData(const std::string& filename) const;
        bool saveDecompressedImage(const std::string& filename) const;

        // 获取结果
        QImage getQImage() const;
        cv::Mat getCVMat() const;
        const std::vector<unsigned char>& getCompressedData() const;
        const std::vector<unsigned char>& getDecompressedData() const;

        // 批量处理
        CompressionResult compressFolder(const std::string& inputFolder,
                                        const std::string& outputFolder);
        CompressionResult decompressFolder(const std::string& inputFolder,
                                         const std::string& outputFolder);

        // 静态工具函数
        static bool isImageFile(const std::string& filename);
        static bool isCompressedFile(const std::string& filename);
        static std::vector<std::string> getImageFiles(const std::string& folder);

    private:
        struct ZstdContext {
            ZSTD_CCtx* cctx = nullptr;
            ZSTD_DCtx* dctx = nullptr;
            ~ZstdContext();
        };

        int m_level;
        int m_num_threads;
        ImageFormat m_format;
        std::vector<unsigned char> m_originalData;
        std::vector<unsigned char> m_compressedData;
        std::vector<unsigned char> m_decompressedData;
        mutable QImage m_qImage; // mutable 用于延迟加载
        mutable cv::Mat m_cvMat;

        std::unique_ptr<ZstdContext> m_ctx;

        bool loadImageFile(const std::string& filename);
        bool convertToImageData(const QImage& image);
        bool convertToImageData(const cv::Mat& image);
        CompressionResult compressInternal();
        CompressionResult decompressInternal();
        void clearResults();
        ZstdContext& getContext();
    };

} // namespace zstd_compressor

#endif // ZSTDBMPCOMPRESSOR_H