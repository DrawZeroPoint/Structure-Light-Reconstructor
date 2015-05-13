#ifndef MFRECONSTRUCT_H
#define MFRECONSTRUCT_H

#include <QObject>

#include <opencv/cv.h>

#include "virtualcamera.h"
#include "pointcloudimage.h"
#include "stereorect.h"

class MFReconstruct : public QObject
{
    Q_OBJECT
public:
    explicit MFReconstruct(QObject *parent = 0);
    void getParameters(int scansn, int scanw, int scanh,  int camw, int camh, int blackt, int whitet, QString savePath);
    bool runReconstruction();

    PointCloudImage *points3DProjView;

private:
    int scanSN;//表示当前重建的扫描数据序列号，也是输出模型的序列号

    QString savePath_;//same as projectPath
    QString *calibFolder;
    QString *scanFolder;
    QString *imgPrefix;
    QString imgSuffix;

    int numberOfImgs;
    int numOfColBits;

    int blackThreshold;
    int whiteThreshold;

    cv::Mat mask;//matrix with vals 0 and 1 , CV_8U , uchar
    cv::Mat decRows;
    cv::Mat decCols;
    cv::vector<cv::Mat> camImgs;

    cv::vector<float> **camsPixels;
    cv::vector<float> *camPixels;

    bool pathSet;
    int cameraWidth;
    int cameraHeight;
    int scan_w;
    int scan_h;

    VirtualCamera *camera;//general functions use this instead of camera1 or camera2
    VirtualCamera	*cameras;
    stereoRect *sr;

    bool loadCameras();
    bool loadCamImgs(QString folder, QString prefix, QString suffix);
    void unloadCamImgs();
    void computeShadows();
    void decodePatterns();
    void getPhase(int row, int col, float &phase);
    void triangulation(cv::vector<float> *cam1Pixels, VirtualCamera cameras1, cv::vector<float> *cam2Pixels, VirtualCamera cameras2);

signals:

public slots:

};

#endif // MFRECONSTRUCT_H
