#include "reconstruct.h"
#include <QMessageBox>

bool processleft = true;

Reconstruct::Reconstruct(bool useEpi)
{
    numOfCams = 2;
    mask = NULL;
    decRows = NULL;
    decCols = NULL;
    points3DProjView = NULL;
    autoContrast_ = false;
    cameras = new  VirtualCamera[2];//生成virtualcamera的两个实例，保存在数组cameras[2]
    camsPixels = new cv::vector<cv::Point>*[2];
    camsPixels_GE = new cv::vector<int>*[2];
    calibFolder = new QString[2];
    scanFolder = new QString[2];
    imgPrefix = new QString[2];
    pathSet = false;
    imgSuffix = ".png";//这里暂时认为图片后缀为.png
    EPI = useEpi;
}
Reconstruct::~Reconstruct()
{
    unloadCamImgs();
    if (points3DProjView)
        delete points3DProjView ;
    if (EPI)
        delete sr;
}

void Reconstruct::enableRaySampling()
{
    raySampling_ = true;
}

void Reconstruct::disableRaySampling()
{
    raySampling_ = false;
}

void Reconstruct::setBlackThreshold(int val)
{
    blackThreshold = val;
}

void Reconstruct::setWhiteThreshold(int val)
{
    whiteThreshold = val;
}

///
/// \brief Reconstruct::decodePaterns 由runReconstruction内部调用，用来求解图像点格雷码对应的投影区域十进制坐标
///
void Reconstruct::decodePaterns()
{
    int w = cameraWidth;
    int h = cameraHeight;
    cv::Point projPixel;//这个变量储存了相片上(w,h)点在投影区域上的坐标projPixel.x，projPixel.y
    for(int col = 0; col < w; col++){
        for(int row = 0; row < h; row++){
            ///mask是根据相机拍摄的图片生成的，因此其大小就是w*h
            if(mask.at<uchar>(row, col)){//if the pixel is not shadow reconstruct
                bool error = getProjPixel(row, col, projPixel);//get the projector pixel for camera (i,j) pixel
                if(error){
                    mask.at<uchar>(row, col) = 0;//进一步补充遮罩区域，相机视野内不属于投影区域的部分都被过滤掉
                    continue;
                }
                camPixels[ac(projPixel.x, projPixel.y)].push_back(cv::Point(col, row));
            }
        }
    }
}

///
/// \brief Reconstruct::decodePatterns_GE 用于格雷码+极线校正解码，由runReconstruction_GE内部调用
///
void Reconstruct::decodePatterns_GE()
{
    int w = cameraWidth;
    int h = cameraHeight;
    int xDec;//这个变量储存了相片上(w,h)点在投影区域上的坐标projPixel.x，projPixel.y
    for(int row = 0; row < h; row++){
        for(int col = 0; col < w; col++){
            ///mask是根据相机拍摄的图片生成的，因此其大小就是w*h
            if(mask.at<uchar>(row, col)){//if the pixel is not shadow reconstruct
                bool error = getProjPixel_GE(row, col, xDec);
                if(error){
                    mask.at<uchar>(row, col) = 0;//进一步补充遮罩区域，相机视野内不属于投影区域的部分都被过滤掉
                    continue;
                }
                camPixels_GE[(row*cameraWidth+col)].push_back(xDec);
            }
        }
    }
}

bool Reconstruct::loadCameras()//Load calibration data into camera[i]
{
    bool loaded;
    for(int i = 0; i < 2; i++)//这里为了处理方便将numofcam直接替换为2
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

        path = savePath_;
        path += "/calib/H1_mat.txt";
        cameras[i].loadHomoMatrix(path, 1);

        path = savePath_;
        path += "/calib/H2_mat.txt";
        cameras[i].loadHomoMatrix(path, 2);

        cameras[i].height = 0;
        cameras[i].width = 0;
    }
    return loaded;
}

bool Reconstruct::loadCamImgs(QString folder, QString prefix, QString suffix)//load camera images
{
    cv::Mat tmp;
    if(!camImgs.empty())
        unloadCamImgs();
    if (EPI)
        sr->calParameters();//若采用极线校正，事先计算参数

    for(int i = 0; i < numberOfImgs; i++){
        QString path;
        path = folder;//这里folder要达到left/right一层
        path += prefix + QString::number(i) + suffix;
        tmp.release();

        tmp = cv::imread(path.toStdString(),0);//flag=0 Return a grayscale image

        if (EPI){
            if (processleft){//第一次调用loadImg时认为是加载左相机图像
                sr->doStereoRectify(tmp,true);
                //cv::imwrite(path.toStdString(),tmp);是否保存校正后的图像
            }
            else{
                sr->doStereoRectify(tmp,false);
                //cv::imwrite(path.toStdString(),tmp);
            }
        }

        if(tmp.empty()){
            QMessageBox::warning(NULL,"Warning","Images not found!");
            break;
        }
        else{
            if(autoContrast_){
                Utilities::autoContrast(tmp,tmp);
            }
            camImgs.push_back(tmp);
        }

        if(camera->width == 0){
            camera->height = camImgs[0].rows;
            camera->width = camImgs[0].cols;
        }
    }
    color = camImgs[0];
    processleft = !processleft;//每调用一次加载图像都对是否处理左图像取反
    return !tmp.empty();
}


void Reconstruct::unloadCamImgs()//unload camera images
{
    if(camImgs.size()){
        for(int i = 0; i<numberOfImgs; i++){
            camImgs[i].release();
        }
    }
    camImgs.clear();
}


void Reconstruct::computeShadows()
{
    int w = camera->width;
    int h = camera->height;
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


bool Reconstruct::runReconstruction()
{
    bool runSucess = false;
    GrayCodes grays(scan_w, scan_h, false);//scan_w scan_h get var getparameter
    numOfColBits = grays.getNumOfColBits();
    numOfRowBits = grays.getNumOfRowBits();
    numberOfImgs = grays.getNumOfImgs();

    for(int i = 0; i < numOfCams; i++){
        cameras[i].position = cv::Point3f(0,0,0);//findProjectorCenter();
        cam2WorldSpace(cameras[i],cameras[i].position);
        camera = &cameras[i];//将position属性已转化到世界坐标系的cameras[i]赋给camera
                                       //在此之前camera相当于一个temp，注意二者单复数有区别
        camsPixels[i] = new cv::vector<cv::Point>[scan_h*scan_w];
        camPixels = camsPixels[i];
        runSucess = loadCamImgs(scanFolder[i], imgPrefix[i], imgSuffix);
        ///截至这一步，实例camera的position、width、height属性已被赋值，camera对应cameras[i]

        if(!runSucess)//如果加载图片失败，中断
            break;
        else{
            if (haveColor){
                colorImgs.push_back(cv::Mat());
                colorImgs[i] = color;//在loadCamImgs中生成了color
            }
            computeShadows();
            decodePaterns();
            unloadCamImgs();
        }
    }
    if(runSucess){
        points3DProjView = new PointCloudImage(scan_w, scan_h, haveColor);
        triangulation(camsPixels[0],cameras[0],camsPixels[1],cameras[1]);
    }
    return runSucess;
}

///
/// \brief Reconstruct::runReconstruction_GE
/// \return 重建是否成功
///
bool Reconstruct::runReconstruction_GE()
{
    bool runSucess = false;
    GrayCodes grays(scan_w, scan_h, true);//scan_w scan_h get var getparameter
    numOfColBits = grays.getNumOfColBits();
    numberOfImgs = grays.getNumOfImgs();

    for(int i = 0; i < numOfCams; i++){
        cameras[i].position = cv::Point3f(0,0,0);//findProjectorCenter();
        cam2WorldSpace(cameras[i],cameras[i].position);
        camsPixels_GE[i] = new cv::vector<int>[cameraHeight * cameraWidth];//将每个相机图像中的每个像素在投影区域中的横坐标记录

        ///camera在loadCamImgs中进行了赋值
        camera = &cameras[i];
        runSucess = loadCamImgs(scanFolder[i], imgPrefix[i], imgSuffix);
        camPixels_GE = camsPixels_GE[i];

        ///截至这一步，实例camera的position、width、height属性已被赋值，camera对应cameras[i]

        if(!runSucess)//如果加载图片失败，中断
            break;
        else{
            if (haveColor){
                colorImgs.push_back(cv::Mat());
                colorImgs[i] = color;//在loadCamImgs中生成了color
            }
            computeShadows();
            decodePatterns_GE();//对camPixels_GE进行了赋值
            unloadCamImgs();
        }
    }
    if(runSucess){
        points3DProjView = new PointCloudImage(scan_w, scan_h, haveColor); //最后一个bool值代表是否上色
        triangulation_ge(camsPixels_GE[0],cameras[0],camsPixels_GE[1],cameras[1]);
    }
    return runSucess;
}


void Reconstruct::cam2WorldSpace(VirtualCamera cam, cv::Point3f &p)//convert a point from camera to world space
{
    cv::Mat tmp(3,1,CV_32F);
    cv::Mat tmpPoint(3,1,CV_32F);
    tmpPoint.at<float>(0) = p.x;
    tmpPoint.at<float>(1) = p.y;
    tmpPoint.at<float>(2) = p.z;
    tmp = -cam.rotationMatrix.t() * cam.translationVector ;
    tmpPoint = cam.rotationMatrix.t() * tmpPoint;
    p.x = tmp.at<float>(0) + tmpPoint.at<float>(0);
    p.y = tmp.at<float>(1) + tmpPoint.at<float>(1);
    p.z = tmp.at<float>(2) + tmpPoint.at<float>(2);
}


bool Reconstruct::getProjPixel(int row, int col, cv::Point &p_out)//for a (x,y) pixel of the camera returns the corresponding projector pixel
{
    cv::vector<bool> grayCol;
    cv::vector<bool> grayRow;
    bool error = false;
    int xDec, yDec;

    ///prosses column images
    for(int count = 0; count < numOfColBits; count++){
        ///get pixel intensity for regular pattern projection and it's inverse
        double val1, val2;
        val1 = Utilities::matGet2D(camImgs[count * 2 + 2], row, col);
        val2 = Utilities::matGet2D(camImgs[count * 2 + 2 +1], row, col);
        ///check if intensity deference is in a valid rage
        if(abs(val1 - val2) < whiteThreshold )
            error = true;
        ///determine if projection pixel is on or off
        if(val1 > val2)
            grayCol.push_back(1);
        else
            grayCol.push_back(0);
    }
    xDec = GrayCodes::grayToDec(grayCol);//由灰度序列grayCol求解其对应的十进制数xDec
    ///prosses row images
    for(int count=0; count < numOfRowBits; count++)
    {
        double val1, val2;
        val1 = Utilities::matGet2D(camImgs[count*2+2+numOfColBits*2], row, col);
        val2 = Utilities::matGet2D(camImgs[count*2+2+numOfColBits*2+1], row, col);
        if(abs(val1-val2) < whiteThreshold )  //check if the difference between the values of the normal and it's inverce projection image is valid
            error = true;
        if(val1 > val2)
            grayRow.push_back(1);
        else
            grayRow.push_back(0);
    }
    ///decode
    yDec = GrayCodes::grayToDec(grayRow);

    if((yDec > scan_h || xDec > scan_w)){
        error = true;//求出的xy坐标超出了投影范围，说明不是投影点，将其遮罩
    }
    p_out.x = xDec;//返回相机照片上像素点在投影仪投影范围内的对应十进制坐标
    p_out.y = yDec;
    return error;
}



///
/// \brief Reconstruct::getProjPixel_GE 格雷码+极线校正下的图像点身份确定算法
/// \param row 图像点行号
/// \param col 图像点列号
/// \param xDec 图像点在投影区域内的横坐标
/// \return 图像点能否被重建
///
bool Reconstruct::getProjPixel_GE(int row, int col, int &xDec)
{
    cv::vector<bool> grayCol;
    bool error = false;

    ///prosses column images
    for(int count = 0; count < numOfColBits; count++){
        ///get pixel intensity for regular pattern projection and it's inverse
        double val1, val2;
        val1 = Utilities::matGet2D(camImgs[count * 2 + 2], row, col);
        val2 = Utilities::matGet2D(camImgs[count * 2 + 2 +1], row, col);
        ///check if intensity deference is in a valid rage
        if(abs(val1 - val2) < whiteThreshold )
            error = true;
        ///determine if projection pixel is on or off
        if(val1 > val2)
            grayCol.push_back(1);
        else
            grayCol.push_back(0);
    }
    xDec = GrayCodes::grayToDec(grayCol);//由灰度序列grayCol求解其对应的十进制数xDec

    if(xDec > scan_w){
        error = true;//求出的x坐标超出了投影范围，说明不是投影点，将其遮罩
    }
    return error;
}


void Reconstruct::setCalibPath(QString folder, int cam_no )
{
    calibFolder[cam_no] = folder;//projectPath+"/calib/left/"或projectPath+"/calib/right/"
    pathSet = true;
}


void Reconstruct::triangulation(cv::vector<cv::Point> *cam1Pixels, VirtualCamera camera1, cv::vector<cv::Point> *cam2Pixels, VirtualCamera camera2)
{
    int w = scan_w;
    int h = scan_h;
    cv::Mat matCoordTrans(3,4,CV_32F);//定义变换矩阵将当前次扫描坐标系对齐至首次扫描坐标系
    if (scanSN > 0){
        ///加载刚体变换矩阵
        QString loadPath = savePath_ + "/scan/transfer_mat" + QString::number(scanSN) + ".txt";
        camera1.loadMatrix(matCoordTrans, 3, 4, loadPath.toStdString());
    }

    for(int i = 0; i < w; i++){
        for(int j = 0; j < h; j++){
            cv::vector<cv::Point> cam1Pixs,cam2Pixs;
            ///cam1Pixels和cam2Pixels是长度为scan_w*scan_h的向量，元素为各个投影点对应相机图像点在原图像上的坐标
            cam1Pixs = cam1Pixels[ac(i, j)];
            cam2Pixs = cam2Pixels[ac(i, j)];

            if( cam1Pixs.size() == 0 || cam2Pixs.size() == 0)//如果投影区域（i,j）处未对应原图像上点，说明其被遮罩
                continue;

            for(int c1 = 0; c1 < cam1Pixs.size(); c1++)
            {
                cv::Point2f camPixelUD = Utilities::undistortPoints(cv::Point2f(cam1Pixs[c1].x,cam1Pixs[c1].y),camera1);//camera 3d point p for (i,j) pixel
                cv::Point3f cam1Point = Utilities::pixelToImageSpace(camPixelUD,camera1); //convert camera pixel to image space
                cam2WorldSpace(camera1, cam1Point);

                cv::Vec3f ray1Vector = (cv::Vec3f) (camera1.position - cam1Point); //compute ray vector
                Utilities::normalize(ray1Vector);

                for(int c2 = 0; c2 < cam2Pixs.size(); c2++)
                {
                    camPixelUD = Utilities::undistortPoints(cv::Point2f(cam2Pixs[c2].x,cam2Pixs[c2].y),camera2);//camera 3d point p for (i,j) pixel
                    cv::Point3f cam2Point = Utilities::pixelToImageSpace(camPixelUD,camera2); //convert camera pixel to image space
                    cam2WorldSpace(camera2, cam2Point);

                    cv::Vec3f ray2Vector = (cv::Vec3f) (camera2.position - cam2Point); //compute ray vector
                    Utilities::normalize(ray2Vector);

                    cv::Point3f interPoint;
                    cv::Point3f refinedPoint;

                    bool ok = Utilities::line_lineIntersection(camera1.position,ray1Vector,camera2.position,ray2Vector,interPoint);

                    if(!ok)
                        continue;
                    /*
                    float X = interPoint.x;
                    float Y = interPoint.y;
                    float Z = interPoint.z;
                    interPoint.z = -Z;
                    interPoint.x = -Y;
                    interPoint.y = -X;
                    */

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
                }
            }
        }
    }
}


void Reconstruct::triangulation_ge(cv::vector<int> *cam1Pixels, VirtualCamera camera1, cv::vector<int> *cam2Pixels, VirtualCamera camera2)
{
    int width = cameraWidth;
    int height = cameraHeight;

    cv::Mat matCoordTrans(3,4,CV_32F);//定义变换矩阵将当前次扫描坐标系对齐至首次扫描坐标系
    if (scanSN > 0){
        ///加载刚体变换矩阵
        QString loadPath = savePath_ + "/scan/transfer_mat" + QString::number(scanSN) + ".txt";
        camera1.loadMatrix(matCoordTrans, 3, 4, loadPath.toStdString());
    }

    ///直接求解空间相交直线交点坐标法
    /*
    for (int i = 0; i < height;i++){
        for (int j = 0;j < width;j++){
            cv::vector<int> cam1Pix = cam1Pixels[i*cameraWidth + j];//注意这里cam1Pix是一个向量，若类型设为int则出错
            if (cam1Pix.size() == 0)
                continue;
            for (int k = 0;k < width;k++){
                cv::vector<int> cam2Pix = cam2Pixels[i*cameraWidth + k];

                if (cam2Pix.size() == 0)
                    continue;

                if (cam1Pix[0] == cam2Pix[0]){//说明左相机(j,i)点与右相机(k,i)点匹配
                    cv::Point2f camPixelUD = Utilities::undistortPoints(cv::Point2f(j, i),camera1);
                    cv::Point3f cam1Point = Utilities::pixelToImageSpace(camPixelUD,camera1);
                    cam2WorldSpace(camera1, cam1Point);

                    cv::Vec3f ray1Vector = (cv::Vec3f) (camera1.position - cam1Point);
                    Utilities::normalize(ray1Vector);

                    camPixelUD = Utilities::undistortPoints(cv::Point2f(k, i),camera2);
                    cv::Point3f cam2Point = Utilities::pixelToImageSpace(camPixelUD,camera2);
                    cam2WorldSpace(camera2, cam2Point);

                    cv::Vec3f ray2Vector = (cv::Vec3f) (camera2.position - cam2Point);
                    Utilities::normalize(ray2Vector);

                    cv::Point3f interPoint;
                    cv::Point3f refinedPoint;

                    bool ok = Utilities::line_lineIntersection(camera1.position,ray1Vector,camera2.position,ray2Vector,interPoint);

                    if(!ok)
                        continue;

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
    */

    ///根据视差及Q矩阵求解法
    for (int i = 0; i < height;i++){//遍历左图像高度方向
        int kstart = 0;//表示每次遍历k时的起点，在k循环找到匹配后更新为匹配k值
        for (int j = 0;j < width;j++){//遍历左图像宽度方向
            cv::vector<int> cam1Pix = cam1Pixels[i * width + j];//注意这里cam1Pix是一个向量，若类型设为int则出错
            if (cam1Pix.size() == 0)
                continue;
            for (int k = kstart;k < width;k++){//遍历右图像宽度方向
                cv::vector<int> cam2Pix = cam2Pixels[i * width + k];
                if (cam2Pix.size() == 0)
                    continue;
                if (cam1Pix[0] == cam2Pix[0]){//说明左相机(j,i)点与右相机(k,i)点匹配
                    ///以左图像该点二维坐标、对应点视差构建该点二维齐次坐标
                    //cv::Point2f camPixelUDL = Utilities::undistortPoints(cv::Point2f(j, i),camera1);
                    //cv::Point2f camPixelUDR = Utilities::undistortPoints(cv::Point2f(k, i),camera2);
                    //double point2D[] = {camPixelUDL.x, camPixelUDL.y, camPixelUDL.x - camPixelUDR.x, 1};//二维坐标
                    double point2D[] = {j, i, j - k, 1};//二维坐标
                    cv::Mat p2D = cv::Mat(4,1,CV_64F,point2D);//构建坐标矩阵
                    cv::Mat p3D;
                    p3D = sr->Q * p2D;
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
                    if (haveColor){
                        int val = (colorImgs[0].at<uchar>(i,j) + colorImgs[1].at<uchar>(i,k))/2;
                        cv::Vec3i graycolor = cv::Vec3i(val,val,val);
                        points3DProjView->addPoint(i, j, refinedPoint, graycolor);
                    }
                    else
                        points3DProjView->addPoint(i, j, refinedPoint);
                    kstart = k;
                    break;//若左图像某点与右图像点已发生了匹配，则不再检索右图像其余点
                }
                else
                    continue;
            }
        }
    }
}


void Reconstruct::getParameters(int scanw, int scanh, int camw, int camh, bool autocontrast, bool havecolor, QString savePath)
{
    scan_w = scanw;
    scan_h = scanh;
    cameraWidth = camw;
    cameraHeight = camh;
    autoContrast_ = autocontrast;
    savePath_ = savePath;//equal to projectPath
    haveColor = havecolor;

    if (havecolor)
        color = cv::Mat(scanh, scanw, CV_8UC3,cv::Scalar(0));

    if (EPI){
        sr = new stereoRect(savePath_, cv::Size(camw,camh));
        sr->getParameters();
    }

    for(int i = 0; i < 2; i++)
    {
        QString pathI;
        if(i==0){
            pathI = savePath + "/scan/left/";//Load Images for reconstruction
        }
        else{
            pathI = savePath + "/scan/right/";
        }
        camsPixels[i] = NULL;
        camsPixels_GE[i] = NULL;
        scanFolder[i] = pathI;
        if(i == 0)
            imgPrefix[i] = QString::number(scanSN) + "/L";
        else
            imgPrefix[i] = QString::number(scanSN) +"/R";
    }
}
