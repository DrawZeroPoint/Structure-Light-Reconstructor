#include "cameracalibration.h"
#include "utilities.h"

#include <QMessageBox>

CameraCalibration::CameraCalibration()
{
    squareSize.width = 0;
    squareSize.height = 0;
    numOfCamImgs = 14;
    camCalibrated = false;
}

CameraCalibration::~CameraCalibration()
{
    unloadCameraImgs();
}

void CameraCalibration::exportTxtFiles(const char *path, int CAMCALIB_OUT_PARAM)
{
    cv::Mat out;
    switch (CAMCALIB_OUT_PARAM)
    {
    case CAMCALIB_OUT_MATRIX:
        out = camMatrix;
        break;
    case CAMCALIB_OUT_DISTORTION:
        out = distortion;
        break;
    case CAMCALIB_OUT_ROTATION:
        out = rotationMatrix;
        break;
    case CAMCALIB_OUT_TRANSLATION:
        out = translationVector;
        break;
    case CAMCALIB_OUT_FUNDAMENTAL:
        out = fundamentalMatrix;
        break;
    case CAMCALIB_OUT_STATUS:
        out = statusMatrix;
        break;
    case CAMCALIB_OUT_H1:
        out = H1;
        break;
    case CAMCALIB_OUT_H2:
        out = H2;
        break;
#ifdef TEST_STEREO
    case STEREOCALIB_OUT_MATRIXL:
        out = camMatrixL;
        break;
    case STEREOCALIB_OUT_MATRIXR:
        out = camMatrixR;
        break;
    case STEREOCALIB_OUT_DISL:
        out = distortionL;
        break;
    case STEREOCALIB_OUT_DISR:
        out = distortionR;
        break;
    case STEREOCALIB_OUT_R:
        out = R;
        break;
    case STEREOCALIB_OUT_T:
        out = T;
        break;
    case STEREOCALIB_OUT_F:
        out = F;
        break;
#endif
    }
    Utilities::exportMat(path, out);
}


void CameraCalibration::loadCameraImgs(QString fpath)
{
    if (calibImgs.size())
        calibImgs.clear();

    for(int i = 0; i < numOfCamImgs-1; i++)
    {
        //这里假定每个相机的标定图片数为13，folderPath应包括前缀L、R
        QString path = fpath;
        path += QString::number(i+1) + ".png";
        cv::Mat img = cv::imread(path.toStdString());
        if(img.empty()){
            QMessageBox::warning(NULL, QObject::tr("Images Not Found"), QObject::tr("The camera calibration images are not found."));
            return;
        }
        calibImgs.push_back(img);
    }

    QString path = fpath;
    path += "1.png";//用第1幅图作为外部参数标定图像
    extrImg = cv::imread(path.toStdString());
    if(extrImg.empty()){
        QMessageBox::warning(NULL, QObject::tr("Image Not Found"), QObject::tr("The images for extrinsicts calibration are missing."));
        return;
    }
    if(!calibImgs[0].empty())
        camImageSize = calibImgs[0].size();
}

void CameraCalibration::unloadCameraImgs()
{
    for(int i = 0; i < calibImgs.size(); i++)
        calibImgs[i].release();
    extrImg.release();
}


void CameraCalibration::undistortCameraImgPoints(cv::vector<cv::Point2f> points_in,cv::vector<cv::Point2f> &points_out)
{
    cv::undistortPoints(points_in,points_out,camMatrix,distortion);
    float fX = camMatrix.at<double>(0,0);
    float fY = camMatrix.at<double>(1,1);
    float cX = camMatrix.at<double>(0,2);
    float cY = camMatrix.at<double>(1,2);

    for(int j=0; j<points_out.size(); j++)
    {
        points_out[j].x = (points_out[j].x*fX)+cX;
        points_out[j].y = (points_out[j].y*fY)+cY;
    }
}


bool CameraCalibration:: findCornersInCamImg(cv::Mat img, cv::vector<cv::Point2f> &camCorners, cv::vector<cv::Point3f> *objCorners)
{
    cv::Mat img_grey;
    cv::Mat img_copy;
    img.copyTo(img_copy);

    if(!useSymmetric){
        numOfCornersX = 4;
        numOfCornersY = 11;
    }
    else{
        numOfCornersX = 11;//这里按标准双目标定板确定横向和纵向方格数目，进一步应改为从set获取
        numOfCornersY = 9;
    }

    bool found = false;
    cv::cvtColor(img, img_grey, CV_RGB2GRAY);
    img.copyTo(img_copy);

    ///这里尝试采用opencv自带的找圆心功能
    cv::Size patternsize(numOfCornersX, numOfCornersY);

    cv::bitwise_not(img_grey, img_grey);//黑底白色圆圈的标定板需要反相处理

    if(!useSymmetric)
        found = cv::findCirclesGrid(img_grey, patternsize, camCorners,cv::CALIB_CB_ASYMMETRIC_GRID);
    else
        found = cv::findCirclesGrid(img_grey, patternsize, camCorners,cv::CALIB_CB_SYMMETRIC_GRID);

    if(!found){
        return false;
    }
    ///要实现全自动可以屏蔽下面的while循环
#ifdef DEBUG
    cv::drawChessboardCorners(img_copy, patternsize, camCorners, found);
    int key = cv::waitKey(1);
    while(found)
    {
        cv::imshow("Calibration", img_copy);
        key = cv::waitKey(1);
        if(key==27)
            found=false;
        if(key==13)
        break;
    }
#endif

    if(found){
        if(squareSize.height == 0){
            squareSize.height = 20;
            squareSize.width = 20;
        }
    if(!useSymmetric){
        for (int i = 0; i < numOfCornersY; i++){
            for (int j = 0; j < numOfCornersX; j++){
                objCorners->push_back(cv::Point3f(float((2*j + i % 2)*squareSize.width),float(i*squareSize.width),0));
            }
        }
    }
    else{
        for(int i = 0; i<numOfCornersY; i++){
            for(int j = 0; j<numOfCornersX; j++){
                cv::Point3f p;
                p.x = j * squareSize.width;
                p.y = i * squareSize.height;
                p.z = 0;
                objCorners->push_back(p);
            }
        }
    }
        return true;
    }
    else
        return false;
}


int CameraCalibration::extractImageCorners()//返回值大于0说明处理不成功，等于零表示处理成功
{
    if(calibImgs.size() == 0)
        return numOfCamImgs+1;
    imgBoardCornersCam.clear();
    objBoardCornersCam.clear();

    for(size_t i = 0; i < calibImgs.size(); i++){
        cv::vector<cv::Point2f> cCam;
        cv::vector<cv::Point3f> cObj;
        bool found = findCornersInCamImg(calibImgs[i], cCam, &cObj );

        if(!found){
            QString cam = (isleft)?("L"):("R");
            if(QMessageBox::warning(NULL,NULL,tr("Couldn't find circles in image ") + cam + QString::number(i+1)
                    + ", Recapture?",
                    QMessageBox::Yes,
                    QMessageBox::Cancel) == QMessageBox::Yes){
                return i+1;
            }
            else
                return numOfCamImgs+1;//返回未能读取的图像序号
        }

        if(cCam.size()){
            imgBoardCornersCam.push_back(cCam);
            objBoardCornersCam.push_back(cObj);
            if(isleft)
                imgBoardCornersCamL.push_back(cCam);
            else
                imgBoardCornersCamR.push_back(cCam);
        }
    }

    /***********为求解基础矩阵，采样点来自第十二组图片（L12，R12）的角点数据*************/
    if (isleft){
        for (int i = 0; i < numOfCornersX*numOfCornersY; i++){
            findFunLeft.push_back(imgBoardCornersCam[11][i]);
        }
    }
    else{
        for (int i = 0; i < numOfCornersX*numOfCornersY; i++){
            findFunRight.push_back(imgBoardCornersCam[11][i]);
        }
    }
    return 0;
}

int CameraCalibration::calibrateCamera()
{
    //check if corners for camera calib has been extracted
    if(imgBoardCornersCam.size() != numOfCamImgs-1){
        if(!extractImageCorners()){
            return 0;
        }
    }

    cv::vector<cv::Mat> camRotationVectors;
    cv::vector<cv::Mat> camTranslationVectors;

    rms = cv::calibrateCamera(objBoardCornersCam, imgBoardCornersCam, camImageSize, camMatrix, distortion, camRotationVectors,camTranslationVectors,0);
    //rms = cv::calibrateCamera(objBoardCornersCam, imgBoardCornersCam, camImageSize, camMatrix,distortion, camRotationVectors, camTranslationVectors, CV_CALIB_FIX_K4|CV_CALIB_FIX_K5);
    if(isleft){
        undistortCameraImgPoints(findFunLeft,findFunLeft);
        camMatrixL = camMatrix;
        distortionL = distortion;
    }
    else{
        undistortCameraImgPoints(findFunRight,findFunRight);
        camMatrixR = camMatrix;
        distortionR = distortion;
    }
    camCalibrated = true;
    return 1;
}


bool CameraCalibration::findCameraExtrisics()
{
    cv::vector<cv::Point2f> imgPoints;
    cv::vector<cv::Point3f> objPoints3D;
    findCornersInCamImg(extrImg, imgPoints, &objPoints3D);
    cv::Mat rVec;
    //find extrinsics rotation & translation
    bool r = cv::solvePnP(objPoints3D,imgPoints,camMatrix,distortion,rVec,translationVector);
    cv::Rodrigues(rVec,rotationMatrix);
    return r;
}

void CameraCalibration::findFundamental()
{
    fundamentalMatrix = cv::findFundamentalMat(findFunLeft, findFunRight, statusMatrix, cv::FM_RANSAC);
    //cv::stereoRectifyUncalibrated(findFunLeft, findFunRight, fundamentalMatrix, camImageSize, H1, H2);
    cv::stereoRectifyUncalibrated(cv::Mat(findFunLeft), cv::Mat(findFunRight), fundamentalMatrix, camImageSize, H1, H2);
    findFunLeft.clear();
    findFunRight.clear();
#ifdef TEST_STEREO
    rms = stereoCalibrate(objBoardCornersCam,imgBoardCornersCamL,imgBoardCornersCamR,camMatrixL,distortionL,camMatrixR,distortionR
                    ,camImageSize,R,T,E,F);
#endif
}

void CameraCalibration::setSquareSize(cv::Size size_in_mm)
{
    squareSize = size_in_mm;
}

cv::Size CameraCalibration::getSquareSize()
{
    return squareSize;
}

void CameraCalibration::setNumberOfCameraImgs(int num)
{
    numOfCamImgs = num;
}

int CameraCalibration::getNumberOfCameraImgs()
{
    return numOfCamImgs;
}

