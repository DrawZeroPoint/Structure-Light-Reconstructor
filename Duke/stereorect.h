#ifndef STEREORECT_H
#define STEREORECT_H

#include <QObject>

#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/contrib/contrib.hpp>

#include "utilities.h"

class stereoRect : public QObject
{
    Q_OBJECT
public:
    stereoRect(QString projectPath);
    void doStereoRectify(cv::Mat &img, bool isleft);
    void getParameters();

private:
    QString ppath;
    cv::Mat M1, D1, M2, D2, R, R64, T;
    void loadMatrix(cv::Mat &matrix, int rows, int cols, QString file);
};
#endif // STEREORECT_H
