#include "stereorect.h"

stereoRect::stereoRect(QString projectPath)
{
    ppath = projectPath;
}

void stereoRect::getParameters()
{
    QString path;
    path = ppath + "/calib/left/cam_stereo.txt";
    loadMatrix(M1, 3, 3, path);
    path = ppath + "/calib/left/distortion_stereo.txt";
    loadMatrix(D1, 5, 1, path);
    path = ppath + "/calib/right/cam_stereo.txt";
    loadMatrix(M2, 3, 3, path);
    path = ppath + "/calib/right/distortion_stereo.txt";
    loadMatrix(D2, 5, 1, path);
    path = ppath + "/calib/R_stereo.txt";
    loadMatrix(R, 3, 3, path);
    path = ppath + "/calib/T_stereo.txt";
    loadMatrix(T, 3, 1, path);
}

void stereoRect::doStereoRectify(cv::Mat &img, bool isleft)
{
    cv::Size img_size = img.size();
    cv::Mat R1, P1, R2, P2, Q;

    ///该矫正函数在使用时注意两点：
    /// 1、flag不要设为CV_CALIB_ZERO_DISPARITY，而是设为0
    /// 2、所有输入矩阵都采用CV_64F格式，否则出现类型不匹配错误
    cv::stereoRectify(M1, D1, M2, D2, img_size, R, T, R1, R2, P1, P2, Q, 0, -1);

    cv::Mat map11, map12, map21, map22;
    if (isleft)
        cv::initUndistortRectifyMap(M1, D1, R1, P1, img_size, CV_16SC2, map11, map12);
    else
        cv::initUndistortRectifyMap(M2, D2, R2, P2, img_size, CV_16SC2, map21, map22);

    cv::Mat imgr;
    if (isleft)
        cv::remap(img, imgr, map11, map12, cv::INTER_LINEAR);
    else
        cv::remap(img, imgr, map21, map22, cv::INTER_LINEAR);

    img = imgr;
}


void stereoRect::loadMatrix(cv::Mat &matrix, int rows, int cols, QString file)
{
    std:: ifstream in1;
    in1.open(file.toStdString());
    if(!in1)
        return;
    if(!matrix.empty())
        matrix.release();
    matrix = cv::Mat(rows, cols, CV_64F);
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            float val;
            in1>>val;
            Utilities::matSet2D(matrix, i, j, val);
        }
    }
}
