#ifndef MANUALMATCH_H
#define MANUALMATCH_H

#include <QWidget>
#include <QKeyEvent>

// OpenCV
#include <opencv/cv.h>
#include <opencv/highgui.h>

namespace Ui {
class ManualMatch;
}

class ManualMatch : public QWidget
{
    Q_OBJECT

public:
    explicit ManualMatch(QWidget *parent = 0);
    ~ManualMatch();

    cv::Mat leftImage;
    cv::Mat rightImage;
    cv::vector<cv::Point2i> correspond;
    cv::vector<cv::Point2i> refinedCorr;//x表示点ID，y值表示序号
    cv::vector<cv::vector<cv::Point2f>> dotInOrder;

    void setImage();

private:
    Ui::ManualMatch *ui;

    void drawCross(QPainter &p, int x, int y);

    size_t onMark;//表示当前光标停留的待标记点

protected:
    void keyPressEvent(QKeyEvent *e);

private slots:
    void confirmID();
    void finish();//点击完成标记按钮触发的动作
    void reset();//点击重置标记按钮触发的动作
signals:
    void outputdata();//由finish按钮所发出的信号，作用是通知dotmatch对refinedCorr进行处理，使用onfinishmanual()槽
};

#endif // MANUALMATCH_H
