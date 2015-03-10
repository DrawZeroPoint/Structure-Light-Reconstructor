#include "cameracalibration.h"
#include "utilities.h"

#include <QMessageBox>

CameraCalibration::CameraCalibration()
{
    squareSize.width = 0;
    squareSize.height = 0;
    numOfCamImgs = 13;
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


/*****************无效功能*****************
void CameraCalibration::perspectiveTransformation(cv::vector<cv::Point2f> corners_in,cv::Mat homoMatrix, cv::vector<cv::Point3f> &points_out)
{
    for(int i=0; i < corners_in.size(); i++)
    {
        cv::Point3f p;
        double x = corners_in[i].x, y = corners_in[i].y;

        double Z = 1./(homoMatrix.at<double>(6) *x + homoMatrix.at<double>(7)*y + homoMatrix.at<double>(8));
        double X =    (homoMatrix.at<double>(0) *x + homoMatrix.at<double>(1)*y + homoMatrix.at<double>(2))*Z;
        double Y =    (homoMatrix.at<double>(3) *x + homoMatrix.at<double>(4)*y + homoMatrix.at<double>(5))*Z;

        p.x = (float) X;
        p.y = (float) Y;
        p.z = 0;

        points_out.push_back(p);
    }
}


void calib_board_corners_mouse_callback( int event, int x, int y, int flags, void* param )
{
    cv::vector<cv::Point2f> *corners= (cv::vector<cv::Point2f>*) param;
    int ev = event;
    switch( event )
    {

        case CV_EVENT_LBUTTONDOWN:
            if(corners->size() ==4)
                break;
            corners->push_back(cv::Point(x,y));
            break;
    }
}

cv::vector<cv::Point2f>  CameraCalibration::manualMarkCheckBoard(cv::Mat img)
{
    cv::vector<cv::Point2f> corners;
    cv::namedWindow("Mark Calibration Board",CV_WINDOW_NORMAL);
    cv::resizeWindow("Mark Calibration Board",800,600);

    cv::setMouseCallback( "Mark Calibration Board",calib_board_corners_mouse_callback, (void*) &corners);//修改获取conners的方式

    bool ok = false;
    while(!ok)
    {
        corners.clear();
        cv::resizeWindow("Mark Calibration Board",800,600);
        int curNumOfCorners = 0;
        cv::Mat img_copy ;
        img.copyTo(img_copy);

        cv::Point2f rectSize(20,20);
        while(corners.size() < 4)
        {
            //draw selected corners and conection lines
            if(curNumOfCorners < corners.size())
            {
                int s = corners.size();
                cv::rectangle(img_copy,	corners[s-1] - rectSize, corners[s-1] + rectSize,cvScalar(0,0,255),3);
                if(!(corners.size() == 1))
                {
                    cv::line(img_copy, corners[s-1],corners[s-2],cvScalar(0,0,255),3);
                }
                curNumOfCorners++;
            }
            cv::imshow("Mark Calibration Board", img_copy);
            cv::waitKey(2);
        }
        //Draw corners and lines
        cv::rectangle( img_copy,	corners[3] - rectSize, corners[3] + rectSize, cvScalar(0,0,255), 3);
        cv::line(img_copy, corners[3],corners[2],cvScalar(0,0,255),10);
        cv::line(img_copy, corners[3],corners[0],cvScalar(0,0,255),10);
        int key = 0;
        QMessageBox::information(NULL, NULL, tr("Press 'Enter' to continue or 'ESC' to select a new area!"));
        //wait for enter or esc key press
        while( key!=27 && key!=13 )
        {
            cv::imshow("Mark Calibration Board", img_copy );
            key = cv::waitKey();
        }
        //if enter set ok as true to stop the loop or repeat the selection process
        if(key == 13)
            ok = true;
        else
            ok = false;
        img_copy.release();
    }
    cv::destroyWindow("Mark Calibration Board");
    return corners;
}

void image_point_return( int event, int x, int y, int flags, void* param )
{
    CvScalar *point= (CvScalar*) param;
    switch( event )
    {
        case CV_EVENT_LBUTTONDOWN:

            point->val[0]=x;
            point->val[1]=y;
            point->val[2]=1;
            break;
    }
}

float CameraCalibration::markWhite(cv::Mat img)
{
        float white;
        cv::namedWindow("Mark White",CV_WINDOW_NORMAL);
        cv::resizeWindow("Mark White",800,600);
        cv::Scalar point;
        // Set a mouse callback
        cv::setMouseCallback( "Mark White",image_point_return, (void*) &point);

        bool ok = false;
        while(!ok)
        {
            cv::Mat img_copy;
            img.copyTo(img_copy);
            cv::resizeWindow("Mark White",800,600);
            int pointsCount=0;
            point.val[2]=0;
            while(pointsCount==0)
            {
                if(point.val[2]==1)
                {
                    cv::rectangle(img_copy, cvPoint(point.val[0]-10,point.val[1]-10),cvPoint(point.val[0]+10,point.val[1]+10),cvScalar(0,0,255),3);

                    white = img.at<uchar>(point.val[1],point.val[0]);

                    pointsCount++;
                    point.val[2]=0;
                }
                cv::imshow("Mark White", img_copy );
                cv::waitKey(2);
            }
            int key = 0;
            while(key != 27 && key != 13)
            {
                cv::imshow("Mark White", img_copy );
                key=cv::waitKey();
            }
            if(key==13)
                ok=true;
            else
                ok=false;
            img_copy.release();
        }
        cvDestroyWindow("Mark White");
        return white;
}

void CameraCalibration::drawOutsideOfRectangle(cv::Mat img,cv::vector<cv::Point2f> rectanglePoints, float color)
{
    std::vector<cv::Point> corners;
    for(int i = 0; i < rectanglePoints.size(); i++)
    {
        corners.push_back(rectanglePoints[i]);
    }

    cv::Mat mask(img.size(),img.type());
    cv::Mat background(img.size(),img.type());

    mask =  1;
    cv::fillConvexPoly(mask, corners ,cv::Scalar(0));

    background = color;
    background.copyTo(img,mask);
}


void CameraCalibration::manualMarkCalibBoardCorners(cv::Mat img,cv::vector<cv::Point2f> &imgPoints_out, cv::vector<cv::Point2f> &objPoints_out)
{
    cv::Mat img_copy;

    img.copyTo(img_copy);

    //get calibration board corners
    cv::vector<cv::Point2f> imgPoints = manualMarkCheckBoard(img_copy);
    cv::cornerSubPix(img, imgPoints, cvSize(15,15), cvSize(-1,-1), cvTermCriteria(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1));

    //draw on image
    cv::rectangle(img_copy,	imgPoints[0] - cv::Point2f(10,10),imgPoints[0] + cv::Point2f(10,10),cvScalar(0,0,255),3);

    for(int i=0; i<4; i++)
    {
        cv::line(img_copy, cv::Point2f(imgPoints[i].x-20,imgPoints[i].y),cv::Point2f(imgPoints[i].x+20,imgPoints[i].y),cvScalar(255,0,0),3);
        cv::line(img_copy, cv::Point2f(imgPoints[i].x,imgPoints[i].y+20),cv::Point2f(imgPoints[i].x,imgPoints[i].y-20),cvScalar(255,0,0),3);
    }

    cv::line(img_copy, imgPoints[0],imgPoints[1],cvScalar(255,255,255),4);
    cv::line(img_copy, imgPoints[0],imgPoints[1],cvScalar(0,0,255),3);
    cv::line(img_copy, imgPoints[3],imgPoints[0],cvScalar(255,255,255),4);
    cv::line(img_copy, imgPoints[3],imgPoints[0],cvScalar(0,255,0),3);


    cv::namedWindow("Marked Board",CV_WINDOW_NORMAL);
    cv::resizeWindow("Marked Board",800,600);
    cv::imshow("Marked Board", img_copy);

    cv::waitKey(10);
    cv::waitKey(10);

    float xS,yS;
    xS = 10;
    yS = 8;

    if(squareSize.height == 0)
    {
        squareSize.height = 20;
        squareSize.width = 20;
    }

    xS=xS*squareSize.width;
    yS=yS*squareSize.height;

    //set object points real world 2D
    cv::vector<cv::Point2f> objPoints;
    objPoints.push_back(cv::Point2f(0,0));
    objPoints.push_back(cv::Point2f(xS,0));
    objPoints.push_back(cv::Point2f(xS,yS));
    objPoints.push_back(cv::Point2f(0,yS));

    imgPoints_out = imgPoints;
    objPoints_out = objPoints;

    cv::destroyWindow("Marked Board");
}
*/


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
    path += QString::number(numOfCamImgs) + ".png";//用第13幅图作为外部参数标定图像
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
    cv::bitwise_not(img_grey, img_grey);//反相处理
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
                return 14;//返回未能读取的图像序号
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

    rms = cv::calibrateCamera(objBoardCornersCam, imgBoardCornersCam, camImageSize, camMatrix, distortion, camRotationVectors,camTranslationVectors,0,
        cv::TermCriteria( (cv::TermCriteria::COUNT)+(cv::TermCriteria::EPS), 30, DBL_EPSILON) );
    //rms = cv::calibrateCamera(objBoardCornersCam, imgBoardCornersCam, camImageSize, camMatrix,
                                     //distortion, camRotationVectors, camTranslationVectors, CV_CALIB_FIX_K4|CV_CALIB_FIX_K5);
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

