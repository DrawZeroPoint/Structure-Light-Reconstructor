#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>

namespace Ui {
class ImageViewer;
}

class ImageViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = 0);
    ~ImageViewer();

    void showImage(QPixmap img);

protected:
    void contextMenuEvent(QContextMenuEvent *);

private:
    Ui::ImageViewer *ui;

private slots:
    void saveimage();
};

#endif // IMAGEVIEWER_H
