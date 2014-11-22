#include "cameracalibration.h"
#include "utilities.h"

#include <QMessageBox>

CameraCalibration::CameraCalibration()
{
    squareSize.width = 0;
    squareSize.height = 0;
    numOfCamImgs = 12;
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
    }
    Utilities::exportMat(path, out);
}

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

void CameraCalibration::loadCameraImgs(QString fpath)
{
    for(int i = 0; i < 12; i++)
    {
        //这里假定每个相机的标定图片数为12，folderPath应包括前缀L、R
        QString path = fpath;
        path += QString::number(i+1) + ".png";//这里假定每个相机的标定图片数为12，folderPath应包括前缀L、R
        cv::Mat img = cv::imread(path.toStdString());
        if(img.empty())
        {
            QMessageBox::warning(NULL, QObject::tr("Images Not Found"), QObject::tr("The camera calibration images are not found."));
            return;
        }
        calibImgs.push_back(img);
    }

    QString path = fpath;
    path += QString::number(12) + ".png";//暂时用第12幅图作为外部参数标定图像，以后可以多拍一幅
    extrImg = cv::imread(path.toStdString());
    if(extrImg.empty())
    {
        QMessageBox::warning(NULL, QObject::tr("Image Not Found"), QObject::tr("The extrinsicts calibration image is missing."));
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


bool CameraCalibration:: findCornersInCamImg(cv::Mat img,cv::vector<cv::Point2f> *camCorners,cv::vector<cv::Point3f> *objCorners)
{
    cv::Mat img_grey;
    cv::Mat img_copy;
    img.copyTo(img_copy);

    int numOfCornersX = 11;//这里按标准双目标定板确定横向和纵向方格数目，进一步应改为从set获取
    int numOfCornersY = 9;

    bool found = false;
    while(!found)
    {
        cv::cvtColor(img, img_grey, CV_RGB2GRAY);
        img.copyTo(img_copy);

        ///为了实现自动获取角点屏蔽以下代码
        /*
        cv::vector<cv::Point2f> chessBoardCorners = manualMarkCheckBoard(img_copy);//改成自动获取角点
        //ask user to mark a white point on checkboard
        float color = markWhite(img_grey);
        drawOutsideOfRectangle(img_grey,chessBoardCorners, color);
        //show img to user
        cv::imshow("Calibration",img_grey);
        cv::waitKey(20);

        cv::namedWindow("Calibration",CV_WINDOW_NORMAL);
        cv::resizeWindow("Calibration",800,600);
*/

        ///这里尝试采用opencv自带的找圆心功能
        cv::Size patternsize(numOfCornersX, numOfCornersY);
        cv::bitwise_not(img_grey, img_grey);
        found = cv::findCirclesGrid(img_grey, patternsize, *camCorners);//改为检测圆点
        //found = cv::findChessboardCorners(img_grey, cvSize(numOfCornersX,numOfCornersY), *camCorners, CV_CALIB_CB_ADAPTIVE_THRESH );
        cv::drawChessboardCorners(img_copy, patternsize, *camCorners, found);
        int key = cv::waitKey(1);
        if(key==13)
            break;
        //QMessageBox::information(NULL, NULL, QObject::tr("Press 'Enter' to continue or 'ESC' to repeat the procedure."));
        ///要实现全自动可以屏蔽下面的while循环
/*
        while(found)
        {
            cv::imshow("Calibration", img_copy );
            key = cv::waitKey(1);
            if(key==27)
                found=false;
            if(key==13)
            break;
        }
*/
    }
    //if corners found find subPixel
    if(found)
    {
        cv::cvtColor(img, img_grey, CV_RGB2GRAY);
        //find sub pix of the corners
        cv::cornerSubPix(img_grey, *camCorners, cvSize(20,20), cvSize(-1,-1), cvTermCriteria(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1));

        if(squareSize.height == 0)
        {
            squareSize.height = 20;//这里直接定义了方格长宽，后期改为从set获取
            squareSize.width = 20;
        }
        for(int i = 0; i<numOfCornersY ; i++)
        {
            for(int j = 0; j<numOfCornersX; j++)
            {
                cv::Point3f p;
                p.x = j * squareSize.width;
                p.y = i * squareSize.height;
                p.z = 0;
                objCorners->push_back(p);
            }
        }
    }
    cv::destroyWindow("Calibration");
    return found;
}


int CameraCalibration::extractImageCorners()
{
    if(calibImgs.size() == 0)
    {
        return 0;
    }
    imgBoardCornersCam.clear();
    objBoardCornersCam.clear();

    for(int i = 0; i < numOfCamImgs; i++)
    {
        int cornersReturn;
        cv::vector<cv::Point2f> cCam;
        cv::vector<cv::Point3f> cObj;
        findCornersInCamImg(calibImgs[i], &cCam, &cObj );//为加入对图像旋转的处理，这里可能需要根据i设置横、纵向点数
        if(cCam.size())
        {
            imgBoardCornersCam.push_back(cCam);
            objBoardCornersCam.push_back(cObj);
        }
    }
    return 1;
}

int CameraCalibration::calibrateCamera()
{
    //check if corners for camera calib has been extracted
    if(imgBoardCornersCam.size() == 0)
        extractImageCorners();

    cv::vector<cv::Mat> camRotationVectors;
    cv::vector<cv::Mat> camTranslationVectors;

    cv::calibrateCamera(objBoardCornersCam,imgBoardCornersCam,camImageSize,camMatrix, distortion, camRotationVectors,camTranslationVectors,0,
        cv::TermCriteria( (cv::TermCriteria::COUNT)+(cv::TermCriteria::EPS), 30, DBL_EPSILON) );

    camCalibrated = true;
    return 1;
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

bool CameraCalibration::findCameraExtrisics()
{
    cv::vector<cv::Point2f> imgPoints;
    cv::vector<cv::Point3f> objPoints3D;
    findCornersInCamImg(extrImg, &imgPoints, &objPoints3D );
    cv::Mat rVec;
    //find extrinsics rotation & translation
    bool r = cv::solvePnP(objPoints3D,imgPoints,camMatrix,distortion,rVec,translationVector);
    cv::Rodrigues(rVec,rotationMatrix);
    return r;
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

