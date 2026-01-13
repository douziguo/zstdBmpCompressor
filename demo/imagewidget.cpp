#include "imagewidget.h"
#include <QResizeEvent>

ImageWidget::ImageWidget(const QString& title, QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    
    titleLabel = new QLabel(title, this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12pt; }");
    
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("QLabel { background-color: #1e1e1e; border: 1px solid #3c3c3c; }");
    
    layout->addWidget(titleLabel);
    layout->addWidget(imageLabel, 1);
    
    setMinimumSize(400, 300);
}

ImageWidget::~ImageWidget()
{
}

void ImageWidget::setImage(const QImage& image)
{
    originalImage = image;
    updateScaledImage();
}

void ImageWidget::clear()
{
    originalImage = QImage();
    scaledImage = QImage();
    imageLabel->clear();
}

void ImageWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateScaledImage();
}

void ImageWidget::updateScaledImage()
{
    if (originalImage.isNull()) {
        imageLabel->clear();
        return;
    }
    
    int labelWidth = imageLabel->width() - 10;
    int labelHeight = imageLabel->height() - 10;
    
    if (labelWidth <= 0 || labelHeight <= 0) return;
    
    scaledImage = originalImage.scaled(labelWidth, labelHeight, 
        Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    imageLabel->setPixmap(QPixmap::fromImage(scaledImage));
}