#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QLabel>
#include <QVBoxLayout>
#include <QImage>

class ImageWidget : public QWidget
{
    Q_OBJECT

public:
    ImageWidget(const QString& title, QWidget *parent = nullptr);
    ~ImageWidget();

    void setImage(const QImage& image);
    void clear();

private:
    void resizeEvent(QResizeEvent *event) override;
    void updateScaledImage();

    QLabel *titleLabel;
    QLabel *imageLabel;
    QImage originalImage;
    QImage scaledImage;
};

#endif // IMAGEWIDGET_H