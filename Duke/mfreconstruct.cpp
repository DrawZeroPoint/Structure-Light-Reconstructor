#include "mfreconstruct.h"
#include <QMessageBox>

bool processl = true;
float PI = 3.1416;

MFReconstruct::MFReconstruct(QObject *parent) :
    QObject(parent)
{
    mask = NULL;
    decRows = NULL;
    decCols = NULL;
    points3DProjView = NULL;
    cameras = new  VirtualCamera[2];//生成virtualcamera的两个实例，保存在数组cameras[2]
    camsPixels = new cv::vector<float>*[2];
    calibFolder = new QString[2];
    scanFolder = new QString[2];
    imgPrefix = new QString[2];
    pathSet = false;
    imgSuffix = ".png";//这里暂时认为图片后缀为.png
    numOfColBits = 12;
    numberOfImgs = 14;
}


void MFReconstruct::getParameters(int scansn, int scanw, int scanh,
                                  int camw, int camh, int blackt, int whitet, QString savePath)
{
    scanSN = scansn;
    scan_w = scanw;
    scan_h = scanh;
    cameraWidth = camw;
    cameraHeight = camh;
    blackThreshold = blackt;
    whiteThreshold = whitet;
    savePath_ = savePath;//equal to projectPath

    sr = new stereoRect(savePath, cv::Size(camw,camh));
    sr->getParameters();

    for (int i = 0; i < 2; i++){
        camsPixels[i] = NULL;
        QString pathI;
        if (i == 0){
            pathI = savePath + "/scan/left/";//Load Images for reconstruction
        }
        else{
            pathI = savePath + "/scan/right/";
        }
        scanFolder[i] = pathI;
        if (i == 0){
            imgPrefix[i] = QString::number(scanSN) + "/L";
            calibFolder[i] = savePath + "/calib/left/";
        }
        else{
            imgPrefix[i] = QString::number(scanSN) +"/R";
            calibFolder[i] = savePath + "/calib/right/";
        }
    }
    pathSet = true;

    if (!loadCameras())
        QMessageBox::warning(NULL,tr("Get Param"),tr("Load Calibration files failed."));
}


bool MFReconstruct::loadCameras()//Load calibration data into camera[i]
{
    bool loaded;
    for(int i = 0; i < 2; i++)
    {
        QString path;
        path = calibFolder[i];
#ifndef USE_STEREOCALIB_DATA
        path += "cam_matrix.txt";
        loaded = cameras[i].loadCameraMatrix(path);//defined in visualcamera
        if(!loaded)
            break;

        path = calibFolder[i];
        path += "cam_distortion.txt";
        cameras[i].loadDistortion(path);//注意loaddistortion方法加载一个5X1矩阵
#else
        path += "cam_stereo.txt";
        loaded = cameras[i].loadCameraMatrix(path);//defined in visualcamera
        if(!loaded)
            break;
        path = calibFolder[i];
        path += "distortion_stereo.txt";
        cameras[i].loadDistortion(path);
#endif
        path = calibFolder[i];
        path += "cam_rotation_matrix.txt";
        cameras[i].loadRotationMatrix(path);

        path = calibFolder[i];
        path += "cam_trans_vectror.txt";
        cameras[i].loadTranslationVector(path);

        path = savePath_;
        path += "/calib/fundamental_stereo.txt";//测试表明，采用立体标定得到的F效果好于单独标定得到的
        cameras[i].loadFundamentalMatrix(path);

        cameras[i].height = cameraHeight;
        cameras[i].width = cameraWidth;
    }
    return loaded;
}


bool MFReconstruct::loadCamImgs(QString folder, QString prefix, QString suffix)//load camera images
{
    cv::Mat tmp;
    if(!camImgs.empty())
        unloadCamImgs();

    sr->calParameters();

    for(int i = 0; i < numberOfImgs; i++){
        QString path;
        path = folder;//这里folder要达到left/right一层
        path += prefix + QString::number(i) + suffix;
        tmp.release();

        tmp = cv::imread(path.toStdString(),0);//flag=0 Return a grayscale image

        if (processl){//第一次调用loadImg时认为是加载左相机图像
            sr->doStereoRectify(tmp,true);
            //cv::imwrite(path.toStdString(),tmp);
        }
        else{
            sr->doStereoRectify(tmp,false);
            //cv::imwrite(path.toStdString(),tmp);
        }

        if(tmp.empty()){
            QMessageBox::warning(NULL,tr("Load Images"),tr("Scan Images not found!"));
            break;
        }
        else{
            camImgs.push_back(tmp);
        }
    }
    processl = !processl;//每调用一次加载图像都对是否处理左图像取反
    return !tmp.empty();
}


void MFReconstruct::unloadCamImgs()//unload camera images
{
    if(camImgs.size()){
        for(int i = 0; i<numberOfImgs; i++){
            camImgs[i].release();
        }
    }
    camImgs.clear();
}


bool MFReconstruct::runReconstruction()
{
    bool runSucess = false;

    for(int i = 0; i < 2; i++){
        camsPixels[i] = new cv::vector<float>[cameraHeight * cameraWidth];//将每个相机图像中的每个像素在投影区域中的横坐标记录

        ///camera在loadCamImgs中进行了赋值
        camera = &cameras[i];
        runSucess = loadCamImgs(scanFolder[i], imgPrefix[i], imgSuffix);
        camPixels = camsPixels[i];

        ///截至这一步，实例camera的position、width、height属性已被赋值，camera对应cameras[i]

        if(!runSucess)//如果加载图片失败，中断
            break;
        else{
            computeShadows();
            decodePatterns();
            unloadCamImgs();
        }
    }
    if(runSucess){
        points3DProjView = new PointCloudImage(scan_w, scan_h, false); //最后一个bool值代表是否上色，这里改为false
        triangulation(camsPixels[0],cameras[0],camsPixels[1],cameras[1]);
    }
    return runSucess;
}


void MFReconstruct::computeShadows()
{
    int w = cameraWidth;
    int h = cameraHeight;
    mask.release();
    mask = cv::Mat(h, w, CV_8U,cv::Scalar(0));//注意h=行数rows，w=列数cols
    for(int col = 0; col < w; col++){
        for(int row = 0; row < h; row++){
            float blackVal, whiteVal;
            blackVal  = (float) Utilities::matGet2D(camImgs[1], row, col);//camImgs[1]表示全黑图像
            whiteVal  = (float) Utilities::matGet2D(camImgs[0], row, col);//camImgs[0]表示全白图像
            if(whiteVal - blackVal > blackThreshold)//同一像素点在全黑、全白投影下反差大于blackThreshold，说明该点不在阴影里
                Utilities::matSet2D(mask, row, col, 1);
            else
                Utilities::matSet2D(mask, row, col, 0);
        }
    }
}


void MFReconstruct::decodePatterns()
{
    int w = cameraWidth;
    int h = cameraHeight;
    float phase;//表示图像上(w,h)点在绝对相位展开图上的相位值
    cv::Mat out(h,w,CV_8U);
    for(int row = 0; row < h; row++){
        for(int col = 0; col < w; col++){
            ///mask是根据相机拍摄的图片生成的，因此其大小就是w*h
            if(mask.at<uchar>(row, col)){//if the pixel is not shadow reconstruct
                getPhase(row, col, phase);
                camPixels[(row*cameraWidth+col)].push_back(phase);
                out.at<uchar>(row,col) = phase;
            }
        }
    }
    //out.convertTo(out,CV_8UC1);
    cv::imwrite(savePath_.toStdString()+"/p.png",out);
}


void MFReconstruct::getPhase(int row, int col, float &phase)
{
    double P[3];//三组相对相位
    float P12, P23, P123;//展开相位

    ///prosses column images
    for(int count = 0; count < 3; count++){//3表示共进行3次4步相移
        int G1, G2, G3, G4;//点在四步相移图像中的灰度值
        G1 = Utilities::matGet2D(camImgs[4*count + 2], row, col);
        G2 = Utilities::matGet2D(camImgs[4*count + 3], row, col);
        G3 = Utilities::matGet2D(camImgs[4*count + 4], row, col);
        G4 = Utilities::matGet2D(camImgs[4*count + 5], row, col);

        ///计算相对相位，注意结果是弧度制还是角度制，加PI使取值范围为
        /// 0~2PI
        if (G4 == G2 && G1 > G3)
            P[count] = 0;
        else if (G4 == G2 && G1 < G3)
            P[count] = PI;
        else if (G1 == G3 && G4 > G2)
            P[count] = 3*PI/2;
        else if (G1 == G3 && G4 < G2)
            P[count] = PI/2;
        else if (G1 == G3 && G4 == G2)
            Utilities::matSet2D(mask, row, col, 0);
        else if (G1 < G3)
            P[count] = atan(float((G4-G2)/(G1-G3))) + PI;
        else if (G1 > G3 && G4 > G2)
            P[count] = atan(float((G4-G2)/(G1-G3))) + 2*PI;
        else
            P[count] = atan(float((G4-G2)/(G1-G3)));
    }

    ///将相对相位利用外差原理进行相位展开
    P12 = (P[0] > P[1])?(P[0] - P[1]):(P[0] - P[1] + 2*PI);
    P23 = (P[1] > P[2])?(P[1] - P[2]):(P[1] - P[2] + 2*PI);
    P123 = (P12 > P23)?(P12 - P23):(P12 - P23 + 2*PI);
    phase = P123/(2*PI)*255;
}


void MFReconstruct::triangulation(cv::vector<float> *cam1Pixels, VirtualCamera camera1, cv::vector<float> *cam2Pixels, VirtualCamera camera2)
{
    int width = cameraWidth;
    int height = cameraHeight;

    cv::Mat matCoordTrans(3,4,CV_32F);//定义变换矩阵将当前次扫描坐标系对齐至首次扫描坐标系
    if (scanSN > 0){
        ///加载刚体变换矩阵
        QString loadPath = savePath_ + "/scan/transfer_mat" + QString::number(scanSN) + ".txt";
        camera1.loadMatrix(matCoordTrans, 3, 4, loadPath.toStdString());
    }

    for (int i = 0; i < height;i++){//遍历图像高度方向
        for (int j = 0;j < width;j++){//遍历图像宽度方向
            cv::vector<float> cam1Pix = cam1Pixels[i * width + j];//注意这里cam1Pix是一个向量
            if (cam1Pix.size() == 0)
                continue;
            for (int k = 0;k < width;k++){
                cv::vector<float> cam2Pix = cam2Pixels[i * width + k];

                if (cam2Pix.size() == 0)
                    continue;

                if (fabs(cam1Pix[0] - cam2Pix[0]) < 0.1){//说明左相机(j,i)点与右相机(k,i)点匹配
                    ///以左图像该点二维坐标、对应点视差构建该点二维齐次坐标
                    cv::Point2f camPixelUDL = Utilities::undistortPoints(cv::Point2f(j, i),camera1);
                    cv::Point2f camPixelUDR = Utilities::undistortPoints(cv::Point2f(k, i),camera2);
                    double point2D[] = {camPixelUDL.x, camPixelUDL.y, camPixelUDL.x - camPixelUDR.x, 1};//二维坐标
                    cv::Mat p2D = cv::Mat(4,1,CV_64F,point2D);//构建坐标矩阵
                    cv::Mat p3D;
                    p3D = sr->Q * p2D;//此处调试以观察是否正确计算
                    double x = p3D.at<double>(0,0);
                    double y = p3D.at<double>(1,0);
                    double z = p3D.at<double>(2,0);
                    double w = p3D.at<double>(3,0);
                    double ax = x/w;
                    double ay = y/w;
                    double az = z/w;

                    cv::Point3f interPoint(ax,ay,az);
                    cv::Point3f refinedPoint;

                    ///以下判断为多次重建得到的点云拼接做准备
                    if (scanSN > 0){
                        float point[] = {interPoint.x, interPoint.y, interPoint.z, 1};
                        cv::Mat pointMat(4, 1, CV_32F, point);
                        cv::Mat refineMat(3, 1, CV_32F);
                        refineMat = matCoordTrans * pointMat;
                        refinedPoint.x = refineMat.at<float>(0, 0);
                        refinedPoint.y = refineMat.at<float>(1, 0);
                        refinedPoint.z = refineMat.at<float>(2, 0);
                    }
                    else
                        refinedPoint = interPoint;
                    points3DProjView->addPoint(i, j, refinedPoint);
                    break;//若左图像某点与右图像点已发生了匹配，则不再检索右图像其余点
                }
                else
                    continue;
            }
        }
    }
}

