#include "imageviewer.h"
#include "ui_imageviewer.h"

#include <QMenu>
#include <QFileDialog>

bool imageseted = false;

ImageViewer::ImageViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImageViewer)
{
    ui->setupUi(this);
    connect(ui->actionSave_Image,SIGNAL(triggered()),this,SLOT(saveimage()));
}

ImageViewer::~ImageViewer()
{
    delete ui;
}

void ImageViewer::showImage(QPixmap img)
{
    ui->imageLabel->setPixmap(img);
    imageseted=true;
}

void ImageViewer::contextMenuEvent(QContextMenuEvent *)
{
    QList<QAction*> actions;
    actions.push_back(ui->actionSave_Image);
    QCursor cur=this->cursor();
    QMenu *menu=new QMenu(this);
    menu->addActions(actions);
    menu->exec(cur.pos());
}

void ImageViewer::saveimage()
{
    if (imageseted){
        QString dir = QFileDialog::getSaveFileName(this,tr("Save Image"),
                                                   "/home/untitled.png",
                                                   tr("Images (*.png *.jpg)"));
        ui->imageLabel->pixmap()->save(dir);
    }
}
