#include "manualmatch.h"
#include "ui_manualmatch.h"

#include <QImage>
#include <QPainter>
#include <QMessageBox>


QFont textfont("Calibri",50);
QColor gcolor(0,255,0);
QColor rcolor(255,0,0);

ManualMatch::ManualMatch(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ManualMatch)
{
    ui->setupUi(this);

    connect(ui->confirmButton,SIGNAL(clicked()),this,SLOT(confirmID()));
    connect(ui->finishButton,SIGNAL(clicked()),this,SLOT(finish()));
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(hide()));
    connect(ui->resetButton,SIGNAL(clicked()),this,SLOT(reset()));
    //connect(ui->idEdit,SIGNAL(textEdited(QString)),ui->confirmButton,SLOT(setEnabled(bool)));

    onMark = 0;
}

ManualMatch::~ManualMatch()
{
    delete ui;
}

void ManualMatch::setImage()
{
    QImage pimage_1 = QImage(leftImage.data,leftImage.cols,leftImage.rows,QImage::Format_Indexed8);
    QImage pimage_2 = QImage(rightImage.data,rightImage.cols,rightImage.rows,QImage::Format_Indexed8);
    QPixmap pcopy_1 =  QPixmap::fromImage(pimage_1);
    QPixmap pcopy_2 = QPixmap::fromImage(pimage_2);
    QPainter pt_1(&pcopy_1);
    QPainter pt_2(&pcopy_2);
    pt_1.setFont(textfont);
    pt_2.setFont(textfont);

    for(size_t i = 0;i < dotInOrder.size();i++)
    {
        pt_1.setPen(gcolor);
        pt_2.setPen(gcolor);

        drawCross(pt_1, dotInOrder[i][0].x ,dotInOrder[i][0].y);
        drawCross(pt_2, dotInOrder[i][1].x, dotInOrder[i][1].y);

        int ID;
        if (refinedCorr.size()){//根据refinedCorr中的数据（如果有）显示i点ID
            for (size_t r = 0; r < refinedCorr.size(); r++){
                if (i == refinedCorr[r].y)
                    ID = refinedCorr[r].x;
            }
            pt_1.drawText(dotInOrder[i][0].x,dotInOrder[i][0].y,QString::number(ID));
            pt_2.drawText(dotInOrder[i][1].x,dotInOrder[i][1].y,QString::number(ID));
        }
        else{//若refinedPoint还未被赋予空间，根据correspond中的数据显示i点ID
            for (size_t c = 0; c < correspond.size(); c++){
                if (i == correspond[c].y)
                    ID = correspond[c].x;
            }
            pt_1.drawText(dotInOrder[i][0].x,dotInOrder[i][0].y,QString::number(ID));
            pt_2.drawText(dotInOrder[i][1].x,dotInOrder[i][1].y,QString::number(ID));
        }
    }

    ///用红色方框标记当前准备赋予编号的点
    pt_1.setPen(rcolor);
    pt_2.setPen(rcolor);
    pt_1.drawRect(dotInOrder[onMark][0].x-15,dotInOrder[onMark][0].y-15,30,30);
    pt_2.drawRect(dotInOrder[onMark][1].x-15,dotInOrder[onMark][1].y-15,30,30);

    ui->leftImage->setPixmap(pcopy_1);
    ui->rightImage->setPixmap(pcopy_2);

    int ID;
    if (!refinedCorr.empty()){
        if (refinedCorr[onMark].x >= 0){//如果onMark点已经被标记过，则在idEdit中显示ID值
            ID = refinedCorr[onMark].x;
            ui->idEdit->setText(QString::number(ID));
        }
        else
            ui->idEdit->clear();
    }
}

void ManualMatch::confirmID()
{
    if (refinedCorr.size() == 0)
        refinedCorr.resize(dotInOrder.size(),cv::Point2i(-1,-1));

    int id = ui->idEdit->text().toInt();
    cv::Point2i corr;
    corr.x = id;
    corr.y = onMark;
    refinedCorr.at(onMark) = corr;

    if (onMark == dotInOrder.size()-1)
        onMark = 0;
    else
        onMark++;
    ui->current->setText(QString::number(onMark));

    setImage();//根据新的信息重绘图像
}

void ManualMatch::finish()
{
    for (size_t i = 0;i < refinedCorr.size(); i++){
        if (!(refinedCorr[i].x >= 0))
            QMessageBox::warning(NULL,"Manual Match",tr("Point ") + QString::number(i) + tr("hasn't been marked."));
    }
    this->hide();
    emit outputdata();
}

void ManualMatch::reset()
{
    refinedCorr.clear();
    onMark = 0;
    setImage();
}

void ManualMatch::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Enter)
        confirmID();
}

void ManualMatch::drawCross(QPainter &p, int x, int y)
{
    int len = 25;
    p.drawLine(x - len, y, x + len, y);
    p.drawLine(x, y - len, x, y + len);
}



