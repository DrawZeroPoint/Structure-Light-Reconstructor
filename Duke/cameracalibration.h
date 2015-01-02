#ifndef CAMERACALIBRATION_H
#define CAMERACALIBRATION_H

#include <opencv/highgui.h>
#include <opencv/cv.h>
#include <opencv2/calib3d/calib3d.hpp>

#include <QObject>

#define CAMCALIB_OUT_MATRIX 1
#define CAMCALIB_OUT_DISTORTION 2
#define CAMCALIB_OUT_ROTATION 3
#define CAMCALIB_OUT_TRANSLATION 4
#define CAMCALIB_OUT_FUNDAMENTAL 5
#define CAMCALIB_OUT_FUNDI 6

class CameraCalibration :public QObject
{
public:
    CameraCalibration();
    ~CameraCalibration();

    int calibrateCamera();

    void loadCameraImgs(QString fpath);
    void unloadCameraImgs();

    bool findCameraExtrisics();
    void findFundamental();

    void	setSquareSize(cv::Size size);
    cv::Size getSquareSize();

    void	setNumberOfCameraImgs(int num);
    int		getNumberOfCameraImgs();
    void	exportTxtFiles(const char *pathNfileName, int CAMCALIB_OUT_PARAM);
    void	printData();
    int		extractImageCorners();

    cv::Mat camMatrix;
    cv::Mat distortion;
    cv::Mat rotationMatrix;
    cv::Mat translationVector;
    cv::Mat fundamentalMatrix;
    cv::Mat statusMatrix;

    cv::vector<cv::vector<cv::Point2f>> imgBoardCornersCam;

    cv::vector<cv::Point2f> findFunLeft;//存储基础矩阵计算用的左相机图像角点
    cv::vector<cv::Point2f> findFunRight;

    bool isleft;

private:
    bool findCornersInCamImg(cv::Mat camImg,cv::vector<cv::Point2f> *camCorners,cv::vector<cv::Point3f> *objCorners);
    void drawOutsideOfRectangle(cv::Mat img,cv::vector<cv::Point2f> rectanglePoints, float color);

    cv::vector<cv::Point2f> manualMarkCheckBoard(cv::Mat img);
    float markWhite(cv::Mat img);
    void manualMarkCalibBoardCorners(cv::Mat img,cv::vector<cv::Point2f> &imgPoints_out, cv::vector<cv::Point2f> &objPoints_out);

    void perspectiveTransformation(cv::vector<cv::Point2f> corners_in,cv::Mat homoMatrix, cv::vector<cv::Point3f> &points_out);
    void undistortCameraImgPoints(cv::vector<cv::Point2f> points_in,cv::vector<cv::Point2f> &points_out);

    cv::vector<cv::vector<cv::Point3f>> objBoardCornersCam;

    cv::Vector<cv::Mat> calibImgs;
    cv::Mat extrImg;

    cv::Size	squareSize;
    int numOfCamImgs;
    cv::Size camImageSize;
    bool camCalibrated;
};

#endif // CAMERACALIBRATION_H
