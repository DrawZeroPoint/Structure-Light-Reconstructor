#ifndef DOTMATCH_H
#define DOTMATCH_H

#include <QObject>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <math.h>
#include "reconstruct.h"
#include "virtualcamera.h"
#include "utilities.h"

using namespace cv;
using namespace std;

class DotMatch : public QObject
{
    Q_OBJECT
public:

    DotMatch(QObject *parent = 0, QString projectPath = NULL);
    vector<vector<float>> findDot(Mat image, int cam);
    void matchDot(Mat limage, Mat rimage);

    vector<vector<float>> dotInOrder;

    //标记并显示标志点所需数据，int值有6个，依次为
    //左点x，y，右点x，y，是否为已知点(0，1，0代表未知)
    //如果已知，对应的唯一编号
    vector<vector<int>> dotForMark;

    Mat leftImage;
    Mat rightImage;

    int bwThreshold;
    bool firstFind;
    int scanNo;

private:
    void triangleCalculate();
    cv::vector<cv::vector<Point3f> > calFeature(cv::vector<Point3f> dotP);
    bool dotClassify(cv::vector<cv::vector<Point3f> > featureTemp);
    void calMatrix();
    void markPoint();
    Reconstruct *rc;

    cv::vector<Point3f> dotPositionEven;//偶数次扫描所得点的绝对坐标
    cv::vector<Point3f> dotPositionOdd;//奇数次扫描所得点的绝对坐标

    //表示新点与原有点对应关系，如果该点实际为原有点
    //x值为该点在dotFeature中的序号
    //y值为该点在dotPosition（even或odd）中的序号，也就是
    //该点在dotInOrder中的序号
    cv::vector<Point2i> correspondPointEven;
    cv::vector<Point2i> correspondPointOdd;
    cv::vector<cv::vector<cv::Point3f>> dotFeature;
    cv::vector<cv::vector<cv::Point3f>> dotFeatureTemp;

signals:

public slots:

};

#endif // DOTMATCH_H
