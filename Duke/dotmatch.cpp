#include "dotmatch.h"
#include "QMessageBox"

bool cameraLoaded = false;
int YDISTANCE = 4;//两相机标志点Y向距离小于该值认为是同一点
int EDGEUP = 10;//水平方向上标志点边缘黑色宽度上限
int EDGEDOWN = 0;
int MIDDOWN = 2;
int MIDUP = 30;
float EPIPOLARERROR = 0.1;//特征点匹配时极线误差上界
float tolerance = 2;//判断特征值是否相等时的误差

DotMatch::DotMatch(QObject *parent, QString projectPath, bool useManual, bool useepi) :
    QObject(parent)
{
    firstFind = true;//第一次查找标志点默认为基准点
    scanSN = 0;//表示扫描的次数，0表示第一次扫描

    rc = new Reconstruct(false);
    rc->calibFolder = new QString[2];
    rc->savePath_ = projectPath;
    rc->setCalibPath(projectPath +"/calib/left/", 0);
    rc->setCalibPath(projectPath +"/calib/right/", 1);
    rc->cameras = new  VirtualCamera[2];

    path = projectPath;
    useManualMatch = useManual;
    useEpi = useepi;
    if (useepi){
        sr = new stereoRect(projectPath, cv::Size(1280,1024));//此处直接赋值应改为间接
        sr->getParameters();
        sr->calParameters();
    }
}

DotMatch::~DotMatch()
{
    if (useEpi)
        delete sr;
}

///_________________________________________________///
/// \brief DotMatch::findDot
/// \param image 需要从中寻找标记点的图像
/// \param isleft 表示当前处理的图像是左图像还是右图像，
///  根据图像左右对找出的标记点坐标进行去畸变处理
/// \return 标记点坐标，内层vector包含x、y两个float型
///_________________________________________________///
vector<vector<float>> DotMatch::findDot(Mat &image, bool isleft)
{
    ///若使用Q矩阵，则先对图像进行校正
    if (useEpi){
        sr->doStereoRectify(image,isleft);
    }
    vector<vector<float>> dotOutput;//用来存储得到的标志点坐标
    Mat bimage = Mat::zeros(image.size(), CV_8UC1);//二值化后的图像

#ifdef USE_ADAPTIVE_THRESHOLD
    adaptiveThreshold(image,bimage,255,ADAPTIVE_THRESH_MEAN_C,THRESH_BINARY,blocksize,60);
    //blocksize较大可以使处理效果趋向二值化，较小则趋向边缘提取；C值的作用在于抑制噪声
#else
    Mat temp = Mat::zeros(image.size(),CV_8U);
    Mat mask = Mat::zeros(image.size(),CV_8U);
    int T1 = OSTU_Region(image);
    for (size_t i = 0; i < image.rows; i++){
        uchar *data = image.ptr<uchar>(i);
        for (int j = 0;j < image.cols;j++){
            if (data[j] >= T1){
                bimage.at<uchar>(i,j)=255;
                temp.at<uchar>(i,j) = T1;
            }
            else{
                temp.at<uchar>(i,j)=data[j];
                mask.at<uchar>(i,j)=255;
            }
        }
    }
    Mat element = getStructuringElement(MORPH_ELLIPSE, Size(11, 11));
    morphologyEx(mask,mask,MORPH_OPEN,element);
    //cvNamedWindow("1",WINDOW_NORMAL);
    //imshow("1", mask);
    int T2 = OSTU_Region(temp);
    for (size_t i = 0; i < temp.rows; i++){
        uchar *data = temp.ptr<uchar>(i);
        for (int j = 0;j < temp.cols;j++){
            if (data[j] >= T2){
                if (mask.at<uchar>(i,j)>0)
                    bimage.at<uchar>(i,j)=255;
            }
        }
    }
#endif

#ifdef DEBUG
    Mat cimage = Mat::zeros(bimage.size(), CV_8UC3);
    Mat dimage = Mat::zeros(bimage.size(), CV_8UC3);
    cvNamedWindow("Threshold",CV_WINDOW_NORMAL);
    imshow("Threshold",bimage);
    cvWaitKey();
#endif

#ifdef USE_FOUR_POINT
    ///四点匹配法
    vector<vector<float>> alltemp;

    for (int i = 0; i < bimage.rows; i++){
        vector<int> ptemp;
        ptemp.push_back(i);//表示对第i行进行处理
        for (int j = 0; j < bimage.cols - 1; j++){
            if ((bimage.at<uchar>(i, j + 1) - bimage.at<uchar>(i, j)) > 0){
                ptemp.push_back(j);
            }//说明发生了状态跳变0->255
            else if((bimage.at<uchar>(i, j + 1) - bimage.at<uchar>(i, j)) < 0){
                ptemp.push_back(j+1);
            }//255->0
        }

        if (ptemp.size() > 4){
            for (size_t p = 1; p <= ptemp.size() - 4; p++){
                int d1 = ptemp[p+1] - ptemp[p];
                int d2 = ptemp[p+2] - ptemp[p+1];
                int d3 = ptemp[p+3] - ptemp[p+2];
                if (d1 > EDGEDOWN && d1 < EDGEUP && d2 >MIDDOWN && d2 < MIDUP && d3 > EDGEDOWN && d3 < EDGEUP){
                    size_t pointCount = alltemp.size();//表示alltemp中已经存在的点数
                    int match = -1;
                    vector<float> localtemp;//包含3个元素，y值，x左值，x右值
                    localtemp.push_back(ptemp[0]);//y
                    localtemp.push_back(ptemp[p]);//x左
                    localtemp.push_back(ptemp[p+3]);//x右
                    if (pointCount == 0){
                        alltemp.push_back(localtemp);
                    }
                    else{
                        for (int q = 0; q < pointCount; q++){
                            int dy = localtemp[0] - alltemp[q][0];
                            int dx = abs(localtemp[2] - alltemp[q][2]);
                            if (dy < 10 && dx < 4){
                                match = q;//说明alltemp中第q点与当前localtemp中的点来自同一个标记点
                                break;//找到q跳出循环，未找到match=-1
                            }
                        }
                        if (match >= 0){
                            int deltax = (alltemp[match][2] - alltemp[match][1]) - (localtemp[2] - localtemp[1]);
                            if (deltax <= 0){
                                alltemp[match] = localtemp;//deltax<0说明localtemp的x左右值代表的直径更大，即更接近圆心
                            }
                        }
                        else{
                            alltemp.push_back(localtemp);//说明该localtemp中的值是新点
                        }
                    }
                }
            }
        }
    }
#ifdef DEBUG
    for (size_t i=0; i <alltemp.size();i++)
    {
        Point2f p;
        p.x = 0.5*(alltemp[i][1]+alltemp[i][2]);
        p.y = alltemp[i][0];
        circle(cimage,p,10,Scalar(0,255,0));
    }
    cvNamedWindow("Point Found",CV_WINDOW_NORMAL);
    imshow("Point Found",cimage);
    cvWaitKey();
#endif

    vector<Point2f> out = subPixel(bimage, alltemp);//make the coordinate of circle center more accurate

#ifdef DEBUG
    for (size_t i=0; i <out.size();i++)
    {
        circle(dimage,out[i],10,Scalar(0,255,0));
    }
    cvNamedWindow("Point Refined",CV_WINDOW_NORMAL);
    imshow("Point Refined",dimage);
    cvWaitKey();
#endif
    
    for (int i = out.size() - 1; i > -1; i--){
        ///在进行极线约束验证前，对找到的标记点坐标进行去畸变处理
        /// 如果采用了极线校正，则跳过该步骤
        cv::Point2f outUD;
        if (useEpi){
            outUD = out[i];
        }
        else{
            if (isleft)
                outUD = Utilities::undistortPoints(out[i],rc->cameras[0]);//若当前处理左图像，则利用cameras[0]的参数对其进行矫正
            else
                outUD = Utilities::undistortPoints(out[i],rc->cameras[1]);
        }
        vector<float> point;
        point.push_back(outUD.x);
        point.push_back(outUD.y);
        dotOutput.push_back(point);
    }
#else
    ///OpenCV检测
    bd = new BlobDetector();
    vector<Point2d> centers;
    bd->findBlobs(bimage, centers);
/*
    vector<vector<float>> dotOutput;//用来存储得到的标志点坐标
    for(size_t i = 0; i < contours.size()-1; i=i+2)
    {
        size_t count = contours[i].size();
        if( count < 6 )//如果该轮廓中所包含的点不足6个，则忽略
            continue;

        Mat pointsf;
        Mat(contours[i]).convertTo(pointsf, CV_32F);
        RotatedRect box = fitEllipse(pointsf);

        if( MAX(box.size.width, box.size.height) > MIN(box.size.width, box.size.height)*3 )
            continue;
        if(MIN(box.size.width, box.size.height) <= YDISTANCE)
            continue;
        if(box.size.width > 150 || box.size.height > 150)
            continue;

        vector<float> dot;
        bool repeat;
        cv::Point2f boxUD = Utilities::undistortPoints(box.center, rc->cameras[cam]);//这里对由椭圆拟合得到的圆心去畸变
        dot.push_back(boxUD.x);
        dot.push_back(boxUD.y);

        ////以下算法用于对位置十分接近的重复点的删除
        if (dotOutput.size() == 0)
            dotOutput.push_back(dot);//生成的点默认是按照y排序的
        else
        {
            for (int i = 0; i < dotOutput.size(); i++)
            {
                if (fabs(dot[0] - dotOutput[i][0]) < 3)
                    repeat = true;
                else
                    repeat = false;
            }
            if (!repeat)
                dotOutput.push_back(dot);
        }
    }
    */
    for(size_t h = 0;h < centers.size();h++){
        vector<float> dot;
        dot.push_back(centers[h].x);
        dot.push_back(centers[h].y);
        dotOutput.push_back(dot);
    }
#endif
    return dotOutput;
}


////____________________________________________________________////
/// 由MainWindow外部调用函数，对左右图像标记点进行匹配
/// 输入为8位灰度图像，返回是否成功匹配图像
////____________________________________________________________////
bool DotMatch::matchDot(Mat &leftImage, Mat &rightImage)
{
    dotInOrder.clear();
    Mat Lcopy = leftImage;
    Mat Rcopy = rightImage;
    if (scanSN%2 == 0){
        dotPositionEven.clear();
        correspondPointEven.clear();
    }
    else{
        dotPositionOdd.clear();
        correspondPointOdd.clear();
    }
    if (!cameraLoaded){
        cameraLoaded =  rc->loadCameras();
        if (cameraLoaded){
            rc->cameras[0].position = cv::Point3f(0,0,0);//获取相机光心在三维空间中的坐标，为后续求解标记点三维坐标做准备
            rc->cam2WorldSpace(rc->cameras[0], rc->cameras[0].position);  
            rc->cameras[1].position = cv::Point3f(0,0,0);
            rc->cam2WorldSpace(rc->cameras[1], rc->cameras[1].position);
            fundMat = rc->cameras[0].fundamentalMatrix; 
        }
    }
    ///找出左右图像中的标志点
    vector<vector<float>> dotLeft = findDot(leftImage,true);
    vector<vector<float>> dotRight = findDot(rightImage,false);

#ifdef TEST_SURF
    vector<KeyPoint> leftkeypoints;
    vector<KeyPoint> rightkeypoints;
    //当出现无法解析的外部符号时，看lib库是否添加了
    SURF detector(5000);//参数越大找到的角点越少
    detector.detect(leftImage,leftkeypoints);
    detector.detect(rightImage,rightkeypoints);
    /*
    for (size_t i=0; i <leftkeypoints.size();i++){
        circle(Lcopy,leftkeypoints[i].pt,20,Scalar(0,255,0));
    }
    cvNamedWindow("Point Found",CV_WINDOW_NORMAL);
    imshow("Point Found",Lcopy);
    cv::waitKey();
    */
    SurfDescriptorExtractor extractor;
    Mat descriptors_1, descriptors_2;
    extractor.compute( leftImage, leftkeypoints, descriptors_1 );
    extractor.compute( leftImage, rightkeypoints, descriptors_2 );

    // Matching descriptor vectors using FLANN matcher
    FlannBasedMatcher matcher;
    vector< DMatch > matches;
    matcher.match( descriptors_1, descriptors_2, matches );

    double max_dist = 0; double min_dist = 100;

    //-- Quick calculation of max and min distances between keypoints
    for( int i = 0; i < descriptors_1.rows; i++ )
    { double dist = matches[i].distance;
      if( dist < min_dist ) min_dist = dist;
      if( dist > max_dist ) max_dist = dist;
    }

    //-- Draw only "good" matches (i.e. whose distance is less than 2*min_dist,
    //-- or a small arbitary value ( 0.02 ) in the event that min_dist is very
    //-- small)
    //-- PS.- radiusMatch can also be used here.
    vector< DMatch > good_matches;

    for( int i = 0; i < descriptors_1.rows; i++ )
    { if( matches[i].distance <= max(2*min_dist, 0.02) )
      { good_matches.push_back( matches[i]); }
    }

    Mat img_matches;
    drawMatches( leftImage, leftkeypoints, rightImage, rightkeypoints,
                 good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
                 vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

    //-- Show detected matches
    cvNamedWindow("Good Matches",CV_WINDOW_NORMAL);
    imshow( "Good Matches", img_matches );
    cv::waitKey();
#endif

#ifdef DEBUG
    cv::vector<Point2f> dl, dr;
    for (size_t i = 0;i < dotLeft.size(); i++)
    {
        Point2f point;
        point.x = dotLeft[i][0];
        point.y = dotLeft[i][1];
        dl.push_back(point);
    }
    for(size_t i = 0; i < dotRight.size(); i++)
    {
        Point2f point;
        point.x = dotRight[i][0];
        point.y = dotRight[i][1];
        dr.push_back(point);
    }

    std::vector<cv::Vec3f> lines1;
    cv::computeCorrespondEpilines(cv::Mat(dl),1,fundMat,lines1);
    for (vector<cv::Vec3f>::const_iterator it= lines1.begin();
             it!=lines1.end(); ++it) {
             cv::line(Rcopy,cv::Point(0,-(*it)[2]/(*it)[1]),
                             cv::Point(Rcopy.cols,-((*it)[2]+(*it)[0]*Rcopy.cols)/(*it)[1]),
                             cv::Scalar(255,255,255));
    }
    std::vector<cv::Vec3f> lines2;
    cv::computeCorrespondEpilines(cv::Mat(dr),2,fundMat,lines2);
    for (vector<cv::Vec3f>::const_iterator it= lines2.begin();
        it!=lines2.end(); ++it) {
             cv::line(Lcopy,cv::Point(0,-(*it)[2]/(*it)[1]),
                             cv::Point(Lcopy.cols,-((*it)[2]+(*it)[0]*Lcopy.cols)/(*it)[1]),
                             cv::Scalar(255,255,255));
    }
    // Display the images with epipolar lines
    cv::namedWindow("Right Image Epilines (RANSAC)",CV_WINDOW_NORMAL);
    cv::imshow("Right Image Epilines (RANSAC)",Lcopy);
    cv::waitKey();
    cv::namedWindow("Left Image Epilines (RANSAC)",CV_WINDOW_NORMAL);
    cv::imshow("Left Image Epilines (RANSAC)",Rcopy);
    cv::waitKey();
#endif
    ////判断两相机所摄标志点的对应关系
    int k = 0;//dotInOrder中现存点个数
    vector<int> rightmatched;//存储每次匹配点后右图像中被匹配点的序号
    for(size_t i = 0; i < dotLeft.size(); i++){
        float error = 2 * EPIPOLARERROR;
        float diser = 2 * YDISTANCE;
        int bestJ = -1;//与i点最为符合的j点
        for(size_t j = 0; j < dotRight.size();j++){
            if (isBelongTo(j, rightmatched))
                continue;
            bool bingo = false;
            float zlr;
            if(useEpi){
                if (fabs(dotLeft[i][1] - dotRight[j][1]) < YDISTANCE && fabs(dotLeft[i][1] - dotRight[j][1]) < diser){
                    zlr = fabs(dotLeft[i][1] - dotRight[j][1]);
                    bingo = true;
                }
            }
            else{
                float pleft[] = {dotLeft[i][0], dotLeft[i][1], 1.0};//齐次坐标
                float pright[] = {dotRight[j][0], dotRight[j][1], 1.0};
                cv::Mat plmat(1, 3, CV_32F, pleft);
                cv::Mat prmat(1, 3, CV_32F, pright);
                cv::Mat ltor = prmat * fundMat * plmat.t();//cv::Mat rtol = ld * fundMat.t() * rd.t();
                zlr = fabs(ltor.at<float>(0,0));//float zrl = rtol.at<float>(0,0);
                if(zlr < EPIPOLARERROR && zlr < error)
                    bingo = true;
            }
            if(bingo){
                //判断当前点相对于上一点X坐标的正负，如正负不同，则不是对应点
                if (k != 0){//表示不是首次发现标记点
                    bool isbreak = false;
                    for (int p = 0;p < k;p++){
                        int checkLefft = dotLeft[i][0] - dotInOrder[p][0];
                        int checkRight = dotRight[j][0] - dotInOrder[p][2];
                        if(checkLefft * checkRight < -128){//此处阈值取值范围为0~负无穷，过小可能导致错匹配，较大可能导致漏检，可以考虑设为用户变量
                            isbreak = true;
                            break;
                        }
                    }
                    if (!isbreak){
                        bestJ = j;
                    }
                }
                else{
                    bestJ = j;
                }
                if (useEpi)
                    diser = zlr;
                else
                    error = zlr;
            }
        }
        if (bestJ >= 0){
            vector<float> dot;
            dot.push_back(dotLeft[i][0]);
            dot.push_back(dotLeft[i][1]);
            dot.push_back(dotRight[bestJ][0]);
            dot.push_back(dotRight[bestJ][1]);
            dotInOrder.push_back(dot);//每个元素都是包含4个float的向量，依次为左x，y；右x，y

            rightmatched.push_back(bestJ);
            k++;
        }
    }

    ///根据当前扫描次数的奇偶性将标记点三维坐标放入dotPositionOdd(Even)
    bool success = triangleCalculate();//三角计算或许可以用opencv自带的triangulatePoints()函数
    if (!success)
        return false;

    ///如果是第一次扫描，所得到的全部标志点从零编号并保存
    cv::vector<cv::vector<float> > featureTemp;
    if (firstFind){
        dotFeature.clear();
        neighborFeature.clear();
        featureTemp = calFeature(dotPositionEven);
        for (size_t p = 0;p < featureTemp.size(); p++){
            dotFeature.push_back(featureTemp[p]);
            vector<int> neighborTemp = calNeighbor(featureTemp, p);
            neighborFeature.push_back(neighborTemp);
            Point2i corr;
            corr.x = p;
            corr.y = p;
            correspondPointEven.push_back(corr);
        }
        markPoint();
        return true;
    }
    else{
        if (scanSN%2 == 0)
            featureTemp = calFeature(dotPositionEven);
        else
            featureTemp = calFeature(dotPositionOdd);

        bool enoughpoint = dotClassify(featureTemp);
        if (enoughpoint){
            ///如果使用手工匹配，则传入图像及参数
            if (useManualMatch)
                setUpManual(leftImage,rightImage);
            markPoint();
            return true;
        }
        else
            return false;
    }
}

////_____________________________________________________////
/// \brief DotMatch::setUpManual
/// \param LImage 左相机标记点查找图像
/// \param RImage 右相机图像
/// 由matchDot进行内部调用，生成ManualMatch实例并初始化
////_____________________________________________________////
void DotMatch::setUpManual(Mat LImage, Mat RImage)
{
    mm = new ManualMatch();
    connect(mm,SIGNAL(outputdata()),this,SLOT(onfinishmanual()));
    mm->leftImage = LImage;
    mm->rightImage = RImage;
    mm->correspond.clear();
    mm->dotInOrder.clear();
    if (scanSN%2 == 0)
        for (size_t c = 0; c < correspondPointEven.size(); c++){
            mm->correspond.push_back(correspondPointEven[c]);
        }
    else
        for (size_t c = 0; c < correspondPointOdd.size(); c++){
            mm->correspond.push_back(correspondPointOdd[c]);
        }
    for (size_t d = 0; d < dotInOrder.size(); d++){
        Point2f ptleft, ptright;
        ptleft.x = dotInOrder[d][0];
        ptleft.y = dotInOrder[d][1];
        ptright.x = dotInOrder[d][2];
        ptright.y = dotInOrder[d][3];
        cv::vector<cv::Point2f> temp;
        temp.push_back(ptleft);
        temp.push_back(ptright);
        mm->dotInOrder.push_back(temp);
    }
}


////___________________________________________________////
/// \brief DotMatch::activeManual
/// 启动手动识别窗口，外部调用
////___________________________________________________////
void DotMatch::activeManual()
{
    mm->move(60,0);
    mm->show();
    mm->setImage();
}

////______________________________________________________////
/// \brief DotMatch::onfinishmanual
/// 槽函数，由click手工标定面板的finish按钮触发，并按照标记结果
/// 对correspondPoint重新赋值，同时发射receivedmanualmatch信号
/// 由MainWindow的finishmanualmatch槽接收
////______________________________________________________////
void DotMatch::onfinishmanual()
{
    if (scanSN%2 == 0){
        correspondPointEven.clear();
        for (size_t e = 0; e < mm->refinedCorr.size(); e++){
            correspondPointEven.push_back(mm->refinedCorr[e]);
        }
    }
    else{
        correspondPointOdd.clear();
        for (size_t o = 0; o < mm->refinedCorr.size(); o++){
            correspondPointOdd.push_back(mm->refinedCorr[o]);
        }
    }
    delete mm;
    emit receivedmanualmatch();
}

////________________________________________________________////
/// 由MainWindow外部调用，进行匹配点后续处理，包括计算变换矩阵
/// 更新标记点坐标、邻域信息及扫描序列递增
////________________________________________________________////
void DotMatch::finishMatch()
{
    if (!firstFind){
        calMatrix();

        if (scanSN%2 == 0){
            updateDot(correspondPointEven, dotPositionEven, dotPositionOdd);
        }
        else {
            updateDot(correspondPointOdd, dotPositionOdd, dotPositionEven);
        }

        ///利用更新后的dotFeature计算邻域信息
        neighborFeature.clear();
        for (size_t i = 0;i < dotFeature.size();i++){
            vector<int> neighborTemp = calNeighbor(dotFeature, i);
            neighborFeature.push_back(neighborTemp);
        }
        markPoint();//这里再次标记是有必要的，因为之前更新了correspondPoint，因此应反映这些更新
    }
    else
        firstFind = false;
    scanSN++;
}


////______________________________________________________________////
/// 由finishMatch内部调用                                                                       ///
/// 根据计算出的变换矩阵更新已知点数据 dotPositionCurrent 以及 dotFeature   ///
////______________________________________________________________////
void DotMatch::updateDot(cv::vector<Point2i> &correspondPointCurrent, cv::vector<Point3f> &dotPositionCurrent, cv::vector<Point3f> dotPositionFormer)
{
    ///对所有已知点分类进行处理
    cv::vector<Point3f> updatedDotPosition;//用来暂存更新后的标记点绝对坐标，按照ID递增顺序排序
    ///暂时认为新增标记点个数不大于20，设置点初值为z=1000，作为是否后来被赋值的判别条件
    updatedDotPosition.resize(dotFeature.size()+20, Point3f(0,0,1000));

    vector<int> pointKnown;//存储当前次扫描确认为已知的点id

    /// (1)检出的已知点，其唯一编号可根据correspondPointCurrent确定，坐标值可根据dotPosition确定
    /// 将两项写入updatedDotPosition中的对应位置
    for (size_t i = 0; i < correspondPointCurrent.size(); i++){
        updatedDotPosition[correspondPointCurrent[i].x] = dotPositionCurrent[correspondPointCurrent[i].y];
        ///pointKnown中放入点的顺序完全由correspondPointCurrent中对应点的排列顺序决定
        pointKnown.push_back(correspondPointCurrent[i].y);
    }

    ///(2)未被检出的已知点
    ///注意这里的i由于采用的是dotFeature的序号，因此是指已知点的唯一编号，而此时dotFeature还没有进行坐标更新和添加新点
    for (size_t ID = 0; ID < dotFeature.size(); ID++){
        bool Iisdetected = false;//判断dotFeature中的ID号点是否被关联
        for (size_t j = 0; j < correspondPointCurrent.size(); j++){
            if (ID == correspondPointCurrent[j].x){
                //如果correspondPointCurrent中出现了唯一编号ID，说明ID点被找到并被匹配了
                Iisdetected = true;
                break;
            }
        }
        if (!Iisdetected){
            //若i点未被关联，则在当前扫描下可能是未探测到或被识别为疑似点
            ///对i点在当前坐标系下计算坐标以便进一步判断
            //获得i点上一次扫描时的坐标

            float ixf = dotPositionFormer[ID].x;
            float iyf = dotPositionFormer[ID].y;
            float izf = dotPositionFormer[ID].z;

            ///注意pointf将要赋给元素类型为64FC1的矩阵，因此其元素在赋值前必须转换为double型，否则计算出错！
            double pointf[] = {ixf, iyf, izf};
            Mat pointFormer(3,1,CV_64FC1,pointf);//表示i点在先前次扫描坐标系下的坐标
            Mat pointLater(3,1,CV_64FC1);

            pointLater = outRInv * pointFormer + outTInv;//直接通过Horn变换获得变换矩阵

            double ixl = pointLater.at<double>(0, 0);
            double iyl = pointLater.at<double>(1, 0);
            double izl = pointLater.at<double>(2, 0);

            ///判断i点是否为未被探测到的已知点
            bool match = false;//表示ID点是否与疑似点k建立了匹配，默认为否
            int matchNum = 0;//表示与ID点match的最佳k值
            for (size_t k = 0; k < dotPositionCurrent.size(); k++){
                float formerError = 2*tolerance;//表示上一k点与ID点距离偏差
                if (isBelongTo(k, pointKnown))
                    continue;  //通过判断表明k点为疑似点

                double distance = pow((ixl - dotPositionCurrent[k].x), 2) + pow((iyl - dotPositionCurrent[k].y), 2) + pow((izl - dotPositionCurrent[k].z), 2);
                distance = sqrt(distance);//求得的偏差为实际距离，单位mm
                if (distance < 2*tolerance && distance < formerError){
                    ///初步认为k点可能为ID点，但还需要通过邻域检查
                    vector<vector<float>> currentFeature = calFeature(dotPositionCurrent);//计算当前次扫描各点的特征值
                    vector<int> neighborOfK = calNeighbor(currentFeature, k);//利用各点特征值得到k点临近点信息
                    vector<int> filteredNeighbor;//储存k点与所有已知点的远近信息
                    for (size_t z = 0; z < neighborOfK.size(); z++){
                        ///将不是已知的点过滤掉
                        if (isBelongTo(neighborOfK[z], pointKnown)){
                            int IDofZ;//neighborOfK中第z个点对应的ID值，因为该点已经确认为已知，因此ID必然存在
                            for (size_t d=0;d<correspondPointCurrent.size();d++){
                                if (neighborOfK[z] == correspondPointCurrent[d].y)
                                    IDofZ = correspondPointCurrent[d].x;
                            }
                            filteredNeighbor.push_back(IDofZ);
                        }
                    }
                    if (checkNeighbor(neighborFeature[ID], filteredNeighbor)){
                        ///如果邻域检查返回值为真，说明k点邻域情况与ID点相同，表明k点更可能是ID点
                        updatedDotPosition[ID] = dotPositionCurrent[k];
                        formerError = distance;
                        matchNum = k;
                        match = true;//说明疑似点k即原ID点
                        correspondPointCurrent.push_back(Point2i(ID,k));//将k点为已知点ID这一信息添加到对应点数据集
                    }
                }
            }

            if (match){
                pointKnown.push_back(matchNum);
            }
            else{
                ///说明i点与任意k点都不匹配，即i点在当前次扫描中未被看到
                updatedDotPosition[ID].x = ixl;
                updatedDotPosition[ID].y = iyl;
                updatedDotPosition[ID].z = izl;
            }
        }
    }

    ///(3)确定为全新点的点
    size_t offset = 0;
    for (size_t m = 0; m < dotPositionCurrent.size(); m++){
        if (isBelongTo(m, pointKnown))
            continue;
        ///通过测试的点应为真正的全新点
        updatedDotPosition[dotFeature.size() + offset] = dotPositionCurrent[m];
        offset++;
    }

    ///最后将updateDotPosition中的数值赋给dotPosition
    dotPositionCurrent.clear();
    for (size_t n = 0; n < updatedDotPosition.size(); n++){
        if (updatedDotPosition[n].z != 1000){
            dotPositionCurrent.push_back(updatedDotPosition[n]);
        }
        else
            break;
    }

    ///利用新的dotPosition计算dotFeature
    dotFeature.clear();
    dotFeature = calFeature(dotPositionCurrent);
}


////_____________________________________________________________////
///  由matchDot内部调用，进行三角计算
////_____________________________________________________________////
bool DotMatch::triangleCalculate()
{
    cv::Point2f dotLeft;
    cv::Point2f dotRight;
    dotRemove.clear();
    if (dotInOrder.size() < 4){
        QMessageBox::warning(NULL, tr("Trangel Calculate"), tr("Less than 4 point in dotInOrder."));
    }
    for (size_t i = 0; i < dotInOrder.size(); i++){
        dotLeft.x = dotInOrder[i][0];
        dotLeft.y = dotInOrder[i][1];
        dotRight.x = dotInOrder[i][2];
        dotRight.y = dotInOrder[i][3];
        cv::Point3f interPoint;

        if (useEpi){
            double point2D[] = {dotLeft.x, dotLeft.y, dotLeft.x - dotRight.x, 1};//二维坐标
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
            interPoint = cv::Point3f(ax,ay,az);
        }
        else{
            cv::Point3f cam1Point = Utilities::pixelToImageSpace(dotLeft, rc->cameras[0]); //convert camera pixel to image space
            rc->cam2WorldSpace(rc->cameras[0], cam1Point);
            cv::Vec3f ray1Vector = (cv::Vec3f) (rc->cameras[0].position - cam1Point); //compute ray vector
            Utilities::normalize(ray1Vector);

            cv::Point3f cam2Point = Utilities::pixelToImageSpace(dotRight, rc->cameras[1]); //convert camera pixel to image space
            rc->cam2WorldSpace(rc->cameras[1], cam2Point);
            cv::Vec3f ray2Vector = (cv::Vec3f) (rc->cameras[1].position - cam2Point); //compute ray vector
            Utilities::normalize(ray2Vector);

            bool ok = Utilities::line_lineIntersection(rc->cameras[0].position, ray1Vector,rc->cameras[1].position, ray2Vector, interPoint);

            if(!ok){
                dotRemove.push_back(i);
                QMessageBox::warning(NULL, tr("Trangel Calculate"), tr("Point ") + QString::number(i) + tr(" calculation failed."));
                continue;
            }
        }
        //能到达这里的dotInOrder[i]都是通过了三角计算的，因此由其计算得到的interPoint可以存入dotPosition
        //需要注意的是，如果真的出现了i点不能计算交叉点的情况，那么将导致dotPosition中点的序号与点的对应关系与dotInOrder中不同
        if (scanSN%2 == 0)
            dotPositionEven.push_back(interPoint);
        else
            dotPositionOdd.push_back(interPoint);
    }

    if (scanSN%2 == 0){
        if (dotPositionEven.size() < 4){
            QMessageBox::warning(NULL, tr("Trangel Calculate"), tr("Point less than 4. Try adjusting the exposure."));
            return false;
        }
        else{
            for (size_t r=0;r<dotRemove.size();r++){
                dotInOrder.erase(dotInOrder.begin()+dotRemove[r]);//通过这一操作使得dotPosition与dotInOrder序号代表点再次一致
            }
            return true;
        }
    }
    else{
        if (dotPositionOdd.size() < 4){
            QMessageBox::warning(NULL, tr("Trangel Calculate"), tr("Point less than 4. Try adjusting the exposure."));
            return false;
        }
        else{
            for (size_t r=0;r<dotRemove.size();r++){
                dotInOrder.erase(dotInOrder.begin()+dotRemove[r]);
            }
            return true;
        }
    }
}

////_________________________________________________________////
/// 计算每点与其余点之间的距离平方作为该点特征值
/// dotP为标记点空间三维绝对坐标(dotPositionOdd 或 dotPositionEven)
////_________________________________________________________////
cv::vector<cv::vector<float>> DotMatch::calFeature(cv::vector<Point3f> dotP)
{
    cv::vector<cv::vector<float>> featureTemp;
    featureTemp.resize(dotP.size());
    for (size_t i = 0; i < dotP.size() - 1; i++){
        for (size_t j = i + 1; j < dotP.size(); j++){
            float xd = dotP[i].x - dotP[j].x;
            float yd = dotP[i].y - dotP[j].y;
            float zd = dotP[i].z - dotP[j].z;
            float disIJ = pow(xd, 2) + pow(yd, 2) + pow(zd, 2);
            disIJ = sqrtf(disIJ);//存储的特征值为实际距离，单位mm
            featureTemp[i].push_back(disIJ);
            featureTemp[j].push_back(disIJ);
        }
    }
    //返回值是根据三角重建获得的绝对坐标解出的特征值
    //若为初次扫描，直接存入dotFeature，若不是，与dotFeature比较后决定取舍
    return featureTemp;
}


////__________________________________________________________________////
/// 方法1：
/// 根据当前扫描获得的特征值查找其中的已有点并计算变换矩阵                         ////
/// 并将新点加入dotFeature库                                                                       ////
/// 注意featureTemp中元素的序号与dotPositionOdd 或dotPositionEven是对应的       ////
/// 方法2：
/// 三角匹配+1单点匹配
////__________________________________________________________________////
bool DotMatch::dotClassify(cv::vector<cv::vector<float> > featureTemp)
{
    vector<Point2i> correspondPoint;
    /* 效果不好的方法，可删除
    int match = 0;//表示匹配的特征值个数
    int validpoint = 0;//表示找到的一致点个数，小于4则不能生成变换矩阵，则应重新获取
    size_t featureSize = dotFeature.size();

    ///定义featureLib，保存featureTemp中各点所匹配的dotFeature中点的序号及匹配度
    vector<vector<Point2i>> featureLib;//Point2i x值表示dotFeature中点的序号，y值表示匹配度match，凡是匹配度大于2的都保存
    featureLib.resize(featureTemp.size());

    ///未知点遍历循环开始
    for (size_t i = 0; i < featureTemp.size(); i++){

        ///已知点遍历循环开始
        for (size_t ID = 0; ID < featureSize; ID++){

            match = 0;//每次变换dotFeature中的待匹配序列号时，都要将之前的匹配数清零

            ///单个未知点特征值遍历循环开始
            for (size_t p = 0; p < featureTemp[i].size(); p++){

                vector<int> matchedIDValue;//表示已经被匹配的ID点中具体的某个特征值，认为一个特征值只能参与一次匹配
                ///单个已知点特征值遍历循环开始
                for (size_t q = 0; q < dotFeature[ID].size(); q++){
                    if (isBelongTo(q,matchedIDValue))
                        continue;
                    float td = fabs(featureTemp[i][p] - dotFeature[ID][q]);
                    if (td < tolerance){
                        matchedIDValue.push_back(q);
                        match++;
                    }
                }///循环结束

            }///循环结束

            if (match >= 1)///这里若不做限制则是将featureTemp中每一点与dotFeature中每点的匹配度计算出来
                                ///匹配度小于1的点认为是全新点
            {
                featureLib[i].push_back(Point2i(ID,match));
            }
        }///已知点遍历循环结束

    }///未知点循环结束

    ///-------------------------------------------------------------------------------------------------///
    /// 至此featureLib中存储了未知点所对应的可能匹配的已知点及其匹配度，下一步
    /// 是遍历dotFeature各点，查找与各ID点具有最大匹配度的当前扫描序列点

    vector<int> matched;//表示当前次扫描序列下已经找到对应ID的点
    vector<int> matchedID;//表示已经与当前次扫描序列中的点匹配的ID点

    for (size_t ID = 0;ID < featureSize;ID++){
        int bestNum = -1;
        int forematch = 0;
        if (isBelongTo(ID, matchedID))
            continue;//如果ID已经被匹配，则直接考察下一个ID点
        for (size_t i = 0;i < featureLib.size();i++){
            for (size_t s = 0;s < featureLib[i].size();s++){
                if (ID == featureLib[i][s].x){
                    if (featureLib[i][s].y > forematch){
                        bestNum = i;
                        forematch = featureLib[i][s].y;
                    }
                }
            }
        }
        if (bestNum >= 0 && forematch >= 2 && !isBelongTo(bestNum,matched)){//添加了等于2，进一步降低条件
            ///这里如果直接认为bestNum对应ID不妥，因为bestNum可能和其他ID也有较高的匹配度
            /// 如果ID是和bestNum匹配度最高的，那么可以确定二者对应
            int maxmatch = featureLib[bestNum][0].y;
            size_t maxnum = 0;
            for (size_t u = 0;u < featureLib[bestNum].size();u++){
                if (featureLib[bestNum][u].y > maxmatch){
                    maxmatch = featureLib[bestNum][u].y;
                    maxnum = featureLib[bestNum][u].x;
                }
            }
            if (maxnum == ID){
                correspondPoint.push_back(Point2i(ID,bestNum));
                matched.push_back(bestNum);//将bestNum放入已经匹配数组，使其不再参与匹配
                matchedID.push_back(ID);
            }
        }
    }
    ///-----------------------------------------------------------------------------------------------------------///
    */

    if (scanSN%2==0){
        if (!FTTM(correspondPoint,dotPositionEven,dotPositionOdd))
            return false;
    }
    else{
        if (!FTTM(correspondPoint,dotPositionOdd,dotPositionEven))
            return false;
    }

    /// 邻域检查，目的是分辨检出的各点邻域情况是否符合已知，如不符合，则排除该点
    for (size_t i = 0;i < featureTemp.size();i++){//1、遍历featureTemp开始，标号i

        for (size_t c = 0;c < correspondPoint.size();c++){//2、遍历correspondPoint开始，标号c

            if (i == correspondPoint[c].y){//如果featureTemp中序号为i的点对应已知点，则继续

                std::sort(featureTemp[i].begin(),featureTemp[i].end());//对featureTemp[i]中i点与featureTemp中其余各点的距离由小到大进行排序
                vector<int> neighborTemp;

                for (size_t e = 0;e < featureTemp[i].size();e++){//3、遍历featureTemp[i]中各个距离值

                    for (size_t p = 0;p < featureTemp.size();p++){//4、再次遍历featureTemp开始

                        for (size_t q = 0;q < featureTemp[p].size();q++){//5、遍历featureTemp[p]中各值

                            if (featureTemp[i][e] == featureTemp[p][q] && i!=p){//若p点q值等于i点e值，且i不等于p，说明该组特征值是由i、p两点计算出来的

                                for (size_t r = 0;r < correspondPoint.size();r++){//6、再次遍历correspondPoint
                                    if(p == correspondPoint[r].y){//若p点也对应已知点
                                        /// 将p点对应的已知点ID推入neighborTemp，由于之前对featureTemp[i]进行了排序，因此已知点推入的先后顺序
                                        /// 反映了这些已知点与i点的远近
                                        neighborTemp.push_back(correspondPoint[r].x);
                                    }

                                }//6
                            }
                        }//5
                    }//4
                }//3

                /// 至此获得了i点与本次扫描获得的所有其余已知点按由近及远顺序的排列，同时i点也被认为对应已知点correspondPoint[c].x
                //bool pass = true;
                bool pass = checkNeighbor(neighborFeature[correspondPoint[c].x], neighborTemp);
                if (pass){
                    if (scanSN%2 == 0)
                        correspondPointEven.push_back(correspondPoint[c]);
                    else
                        correspondPointOdd.push_back(correspondPoint[c]);
                    //validpoint++;暂时注释掉
                }
            }
        }//2、遍历correspondPoint结束，标号c
    }//1、遍历featureTemp结束，标号i

    /// 至此featureTemp中所有被认为是已知的点都与dotFeature中的点一一对应，并被用来计算变换矩阵
    //if (validpoint > 3){
        return true;
    //}
    /*else{
        QMessageBox::warning(NULL,tr("Dot Classify"),tr("Alignment can't continue due to unenough point."));
        return true;//调试用：这里既是匹配点小于3，也为true，正常应为false
    }*/
}

///_______________________________________________________________________///
/// \brief DotMatch::FTTM fast triangle template matching 快速三角模板匹配
/// \param correspondPoint 输出量，表示已知点与未知点对应关系，x存储ID，y存储id
/// \param dotPositionCurrent 当前检出的未知点坐标
/// \param dotPositionFormer 已知点坐标
/// \return 是否匹配成功（有4个及以上匹配点）
///_______________________________________________________________________///
bool DotMatch::FTTM(cv::vector<Point2i> &correspondPoint, cv::vector<Point3f> dotPositionCurrent, cv::vector<Point3f> dotPositionFormer)
{
    /// 采用三角模板匹配法的标记点识别
    /// 直接根据dotPosition计算三角形边长
    vector<Point2i> corr;//存储首次匹配的三角形顶点
    float minerror = 20;//储存三角形匹配的最小误差，20为大于一般误差上限的初值
    int bestMatchNum = 0;//表示匹配误差最小的一次匹配组序号，corr中连续三个对应点值构成一个匹配组
    int countFinish = 0;//表示已经实现的匹配组数

    for (size_t u=0;u<dotPositionCurrent.size()-2;u++){
        for (size_t v=u+1;v<dotPositionCurrent.size()-1;v++){
            for (size_t w=v+1;w<dotPositionCurrent.size();w++){
                ///取出了u、v、w三个未知点，接下来计算各边长
                float vw = Triangle::calDistance(dotPositionCurrent[v],dotPositionCurrent[w]);
                float uw = Triangle::calDistance(dotPositionCurrent[w],dotPositionCurrent[u]);
                float uv = Triangle::calDistance(dotPositionCurrent[v],dotPositionCurrent[u]);
                ///生成该三角形的实例
                Triangle tri_unknown(u,v,w,vw,uw,uv);
                ///接下来生成已知三角形的实例
                for (size_t k=0;k<dotPositionFormer.size()-2;k++){
                    for (size_t m=k+1;m<dotPositionFormer.size()-1;m++){
                        for (size_t n=m+1;n<dotPositionFormer.size();n++){
                            float mn = Triangle::calDistance(dotPositionFormer[m],dotPositionFormer[n]);
                            float kn = Triangle::calDistance(dotPositionFormer[k],dotPositionFormer[n]);
                            float km = Triangle::calDistance(dotPositionFormer[k],dotPositionFormer[m]);
                            ///生成已知三角形实例
                            Triangle tri_known(k,m,n,mn,kn,km);
                            vector<Point2i> corrtemp;
                            float error=0;
                            bool same = Triangle::copmareTriangle(tri_known,tri_unknown,corrtemp,error);
                            if (same){
                                countFinish++;
                                for (size_t i=0;i<corrtemp.size();i++){
                                    corr.push_back(corrtemp[i]);
                                }
                                if (error < minerror){
                                    minerror = error;
                                    bestMatchNum = countFinish - 1;//例如第一次满足条件，已完成的匹配数countFinish为1，最佳匹配序号为0（从0开始）
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    ///至此如果发现了全等三角形，则corr中至少存在3个对应点，或3的整数倍，取前三个对应点中的2个与其余未知点再组成一个三角形
    if (corr.size() >= 3){//说明至少已知一个三角形
        vector<int> hasmatched;//保存已经匹配的已知点ID
        vector<float> matcherror;//保存各已知点与未知点匹配的匹配误差（如果发生了匹配的话）
        matcherror.resize(dotPositionFormer.size());

        ///从未知点集中取出一点，与已知三角形中的两点组成新的三角形，由于该三角形可能是等腰或等边，因此已知三角形中的两点如不满足
        /// 则更换两点，三点中取两点共有3种情况，由于事先已知三角形不是等边或等腰，因此一定有一条边使新三角形普通
        int oa = 0, ob = 1;//表示已知点中取值相对3*bestMatchNum的偏移量

        for (size_t x = 0;x < dotPositionCurrent.size();x++){
            if (x==corr[3*bestMatchNum].y||x==corr[3*bestMatchNum+1].y||x==corr[3*bestMatchNum+2].y)
                continue;
            ///表示未知点x与已知三角形中的两点构成的三角形是否与任意已知三角形发生了匹配，如果一直未发生匹配
            /// 可能性为1、确实没有任意已知三角形与未知三角形全等，2、未知三角形为等腰或等边三角形
            /// 为排除可能性2，需要对未知三角形中的已知两点进行遍历
            bool noMatch = true;

            for (size_t sq = 0;sq < 3;sq++){//构建对已知三角形三边的遍历
                oa = sq; ob = sq+1;
                if (ob == 3) ob = 0;

                float ol = Triangle::calDistance(dotPositionCurrent[corr[3*bestMatchNum+oa].y],dotPositionCurrent[corr[3*bestMatchNum+ob].y]);
                float xo = Triangle::calDistance(dotPositionCurrent[x],dotPositionCurrent[corr[3*bestMatchNum+oa].y]);
                float xl = Triangle::calDistance(dotPositionCurrent[x],dotPositionCurrent[corr[3*bestMatchNum+ob].y]);
                Triangle tri_unknown(x,corr[3*bestMatchNum+oa].y,corr[3*bestMatchNum+ob].y,ol,xo,xl);//构建包含未知点x和两个已知点的三角形

                for (size_t z = 0;z < dotPositionFormer.size();z++){
                    if (z==corr[3*bestMatchNum].x||z==corr[3*bestMatchNum+1].x||z==corr[3*bestMatchNum+2].x)
                        continue;
                    float lo = Triangle::calDistance(dotPositionFormer[corr[3*bestMatchNum+oa].x],dotPositionFormer[corr[3*bestMatchNum+ob].x]);
                    float zo = Triangle::calDistance(dotPositionFormer[z],dotPositionFormer[corr[3*bestMatchNum+oa].x]);
                    float zl = Triangle::calDistance(dotPositionFormer[z],dotPositionFormer[corr[3*bestMatchNum+ob].x]);
                    Triangle tri_known(z,corr[3*bestMatchNum+oa].x,corr[3*bestMatchNum+ob].x,lo,zo,zl);//构建已知点三角形

                    vector<Point2i> corrtemp;
                    float error = 0;
                    bool same = Triangle::copmareTriangle(tri_known,tri_unknown,corrtemp,error);
                    if (same){
                        noMatch = false;//说明未知三角形普通（非等腰或等边）
                        if (!isBelongTo(z,hasmatched)){//如果ID=z的已知点还没有被匹配
                            correspondPoint.push_back(Point2i(z,x));
                            hasmatched.push_back(z);
                            matcherror[z]=error;
                        }
                        else{//z点已经被匹配，但有新的x点与z的匹配误差小于之前的匹配
                            if (error < matcherror[z]){
                                matcherror[z] = error;
                                for (size_t c=0; c<correspondPoint.size();c++){//更新z的对应未知点序号x
                                    if (correspondPoint[c].x = z)
                                        correspondPoint[c] = Point2i(z,x);
                                }
                            }
                        }
                    }
                }
                if (!noMatch)//如果能够匹配，则不再考虑其他边
                    break;
            }//已知三角形三边遍历结束
        }

        ///将corr中的已知点对存入correspondPoint，只存入匹配误差最小的那个匹配组
        /// 调试时可以在此设置断点，查看corr中具体有多少点，也可以查看correspondPoint已有多少点
        for (size_t i = 3*bestMatchNum; i < 3+3*bestMatchNum; i++){
            correspondPoint.push_back(corr[i]);
        }

        return true;
    }
    else
        return false;
}



////__________________________________________________________________////
/// 邻域计算函数，作用是根据标记点特征（即该点与其余点的距离）                   ////
/// 将其余点按照与该点由近及远的顺序排列，形成一个各点序号的整数数列          ////
/// 并将这一数列作为该点的一个特征，之后检测若疑似为该点但邻域检查出错       ////
/// 则否定检测结果                                                                                     ////
////__________________________________________________________________////
vector<int> DotMatch::calNeighbor(vector<vector<float>> input, int num)
{
    vector<int> output;
    std::sort(input[num].begin(),input[num].end());//注意是升序排列，近点在前，远点在后
    for (size_t e = 0;e < input[num].size();e++){
        for (size_t j = 0;j < input.size();j++){
            for (size_t k = 0;k < input[j].size();k++){
                if (input[num][e] == input[j][k] && num!=j)
                    output.push_back(j);
            }
        }
    }
    return output;
}

////____________________________________________________________////
/// \brief DotMatch::checkNeighbor
/// \param referance 作为参考的ID序列
/// \param needcheck 被检查的ID序列
/// \return 是否通过检查
////____________________________________________________________////
bool DotMatch::checkNeighbor(vector<int> referance, vector<int> needcheck)
{
    int error = 0;
    for (size_t i = 0;i < needcheck.size();i++){
        int flag = -1;
        for (size_t j = 0;j < referance.size();j++){
            if (needcheck[i] == referance[j]){
                flag = j;
            }
        }
        if (flag >= 0){
            for (size_t p = i;p < needcheck.size();p++){
                for (size_t q = 0;q <= flag;q++){
                    if (needcheck[p] == referance[q] && q != flag){
                        referance[q] = -1;//使其不再参与比较
                        error++;
                    }
                }
            }
        }
        else{
            continue;
        }
    }
    if (error < 2)
        return true;//error = 2可能是因对象点误检造成的，不足以说明当前的位置错误
    else
        return false;
}

////______________________________________________________________________________////
/// 通过储存在correspondPointEven及correspondPointOdd中的一致点变换前后坐标计算变换矩阵
////______________________________________________________________________________////
void DotMatch::calMatrix()
{
    if (dotPositionEven.size() == 0 || dotPositionOdd.size() == 0){
        QMessageBox::warning(NULL, tr("Not Enough Data"), tr("Accuqaring point position data failed"));
        return;
    }
    cv::vector<Point3f> pFormer;
    cv::vector<Point3f> pLater;

    if (!useManualMatch){//配合updateDotPosition的方法
        /// 不使用manualmatch时，当前次correspondPoint仅包含之前扫描添加的已知点，不包含
        /// 当次扫描添加的已知点，因此与之前已知点是子集关系，其中所有已知点在之前都已被发现
        if (scanSN%2 == 0){
            for (size_t j = 0; j < correspondPointEven.size(); j++){
                pLater.push_back(dotPositionEven[correspondPointEven[j].y]);//Even中点的顺序为检出顺序
                pFormer.push_back(dotPositionOdd[correspondPointEven[j].x]);//Odd中的点是按照ID排序
            }
        }
        else{
            for (size_t j = 0; j < correspondPointOdd.size(); j++){
                pLater.push_back(dotPositionOdd[correspondPointOdd[j].y]);
                pFormer.push_back(dotPositionEven[correspondPointOdd[j].x]);
            }
        }
    }
    else{
        /// 若采用manualmatch，当前次correspondPoint中还包含当次扫描标记的已知点，这些点在之前
        /// 的扫描中没有出现，也就没有其之前的坐标，因此二者是交集，只能取交集的部分做计算
        for (size_t i = 0; i < correspondPointOdd.size(); i++){
            for (size_t j = 0; j < correspondPointEven.size(); j++){
                if (correspondPointOdd[i].x == correspondPointEven[j].x){
                    if (scanSN%2 == 0){
                        pFormer.push_back(dotPositionOdd[correspondPointOdd[i].x]);//这里做了改动，由y改为x
                        pLater.push_back(dotPositionEven[correspondPointEven[j].y]);
                    }
                    else{
                        pFormer.push_back(dotPositionEven[correspondPointEven[j].x]);
                        pLater.push_back(dotPositionOdd[correspondPointOdd[i].y]);
                    }
                }
            }
        }
    }

    //Horn四元数法求解变换矩阵
    std::vector<double> inpoints;
    std::vector<double> outQuaternion;
    outQuaternion.resize(7);//如果不首先确定大小，可能产生和指针有关的问题

    for(size_t i = 0;i < pFormer.size();i++){
        inpoints.push_back(pFormer[i].x);//经过验证，前面的点为基准，后面的点为待移动点
        inpoints.push_back(pFormer[i].y);//求解出的矩阵是将后面的坐标对齐到前面
        inpoints.push_back(pFormer[i].z);
        inpoints.push_back(pLater[i].x);
        inpoints.push_back(pLater[i].y);
        inpoints.push_back(pLater[i].z);
    }

    mrpt::scanmatching::HornMethod(inpoints,outQuaternion);
    double tx = outQuaternion[0];
    double ty = outQuaternion[1];
    double tz = outQuaternion[2];
    double w = outQuaternion[3];
    double i = outQuaternion[4];
    double j = outQuaternion[5];
    double k = outQuaternion[6];
    double r0 = pow(w,2)+pow(i,2)-pow(j,2)-pow(k,2);
    double r1 = 2*(i*j-w*k);
    double r2 = 2*(i*k+w*j);
    double r3 = 2*(i*j+w*k);
    double r4 = pow(w,2)-pow(i,2)+pow(j,2)-pow(k,2);
    double r5 = 2*(j*k-w*i);
    double r6 = 2*(k*i-w*j);
    double r7 = 2*(k*j+w*i);//Horn原论文此处疑有误，正确的参考www.j3d.org/matrix_faq/matrfaq_latest.html
    double r8 = pow(w,2)-pow(i,2)-pow(j,2)+pow(k,2);
    double data[] = {r0,r1,r2,tx,r3,r4,r5,ty,r6,r7,r8,tz};
    Mat outMat(3,4,CV_64FC1,data);

    inpoints.clear();
    outQuaternion.clear();
    outQuaternion.resize(7);//如果不首先确定大小，可能产生和指针有关的问题

    for(size_t i = 0;i < pLater.size();i++){
        inpoints.push_back(pLater[i].x);//经过验证，前面的点为基准，后面的点为待移动点
        inpoints.push_back(pLater[i].y);//求解出的矩阵是将后面的坐标对齐到前面
        inpoints.push_back(pLater[i].z);
        inpoints.push_back(pFormer[i].x);
        inpoints.push_back(pFormer[i].y);
        inpoints.push_back(pFormer[i].z);
    }

    mrpt::scanmatching::HornMethod(inpoints,outQuaternion);
    tx = outQuaternion[0];
    ty = outQuaternion[1];
    tz = outQuaternion[2];
    w = outQuaternion[3];
    i = outQuaternion[4];
    j = outQuaternion[5];
    k = outQuaternion[6];
    r0 = pow(w,2)+pow(i,2)-pow(j,2)-pow(k,2);
    r1 = 2*(i*j-w*k);
    r2 = 2*(i*k+w*j);
    r3 = 2*(i*j+w*k);
    r4 = pow(w,2)-pow(i,2)+pow(j,2)-pow(k,2);
    r5 = 2*(j*k-w*i);
    r6 = 2*(k*i-w*j);
    r7 = 2*(k*j+w*i);//Horn原论文此处疑有误，正确的参考www.j3d.org/matrix_faq/matrfaq_latest.html
    r8 = pow(w,2)-pow(i,2)-pow(j,2)+pow(k,2);
    double datainv[] = {r0,r1,r2,tx,r3,r4,r5,ty,r6,r7,r8,tz};
    Mat outMatInv(3,4,CV_64FC1,datainv);

    cv::Range rangeR(0,3);
    cv::Range rangeT(3,4);

    outR = outMat(cv::Range::all(),rangeR).clone();//注意必须对等号右侧深拷贝才能使用
    outT = outMat(cv::Range::all(),rangeT).clone();
    outRInv = outMatInv(cv::Range::all(),rangeR).clone();
    outTInv = outMatInv(cv::Range::all(),rangeT).clone();

    if (scanSN == 1){
        Mat transFormer(3,4,CV_64FC1);
        transFormer = outMat(cv::Range::all(),cv::Range::all());

        ///从实验的情况来看，如果一个矩阵是继承自其他矩阵的局部块，那么若要利用该矩阵进行计算
        /// 必须对所引用的局部块进行深拷贝
        matRotation = transFormer(cv::Range::all(),rangeR).clone();
        matTransform = transFormer(cv::Range::all(),rangeT).clone();

        QString outTransPath = path + "/scan/transfer_mat1" + ".txt";
        Utilities::exportMat(outTransPath.toLocal8Bit(), transFormer);
    }
    else {
        Mat transR(3,3,CV_64FC1);//存放待输出矩阵的R部分
        Mat transT(3,1,CV_64FC1);//存放待输出矩阵的T部分

        /// 在以下计算中，等号左值为初始化了的空矩阵，右值全部来自于局部块的深拷贝
        /// 因此不依赖于原矩阵的值，因此计算能够成立，而直接采用局部块进行计算是不行的！
        transR = matRotation * outR;//例如，当scanSN=2时，=R1*R2
        transT = matRotation * outT + matTransform;//=R1*T2+T1
        matRotation = transR;//更新数值为R1*R2
        matTransform = transT;//更新数值为//R1*T2+T1

        QString outTransPath = path + "/scan/transfer_mat" + QString::number(scanSN) + ".txt";
        Utilities::exportMatParts(outTransPath.toLocal8Bit(), transR, transT);
    }
}

/*
////__________________________////
///         由calMatrix内部调用         ///
///         Horn法求解变换矩阵        ///
////__________________________////
void DotMatch::hornTransform(double &data[], cv::vector<Point3f> target, cv::vector<Point3f> move)
{
    std::vector<double> inpoints;
    for(size_t i = 0;i < target.size();i++){
        inpoints.push_back(target[i].x);//经过验证，前面的点为基准，后面的点为待移动点
        inpoints.push_back(target[i].y);//求解出的矩阵是将后面的坐标对齐到前面
        inpoints.push_back(target[i].z);
        inpoints.push_back(move[i].x);
        inpoints.push_back(move[i].y);
        inpoints.push_back(move[i].z);
    }
    std::vector<double> outQuaternion;
    outQuaternion.resize(7);//如果不首先确定大小，可能产生和指针有关的问题
    mrpt::scanmatching::HornMethod(inpoints,outQuaternion);
    double tx = outQuaternion[0];
    double ty = outQuaternion[1];
    double tz = outQuaternion[2];
    double w = outQuaternion[3];
    double i = outQuaternion[4];
    double j = outQuaternion[5];
    double k = outQuaternion[6];
    double r0 = pow(w,2)+pow(i,2)-pow(j,2)-pow(k,2);
    double r1 = 2*(i*j-w*k);
    double r2 = 2*(i*k+w*j);
    double r3 = 2*(i*j+w*k);
    double r4 = pow(w,2)-pow(i,2)+pow(j,2)-pow(k,2);
    double r5 = 2*(j*k-w*i);
    double r6 = 2*(k*i-w*j);
    double r7 = 2*(k*j+w*i);//Horn原论文此处疑有误，正确的参考www.j3d.org/matrix_faq/matrfaq_latest.html
    double r8 = pow(w,2)-pow(i,2)-pow(j,2)+pow(k,2);
    data[] = {r0,r1,r2,tx,r3,r4,r5,ty,r6,r7,r8,tz};
}
*/

////_________________________________________________________////
/// \brief DotMatch::markPoint
/// 内部调用函数
/// 为全局变量dotForMark赋值
////_________________________________________________________////
void DotMatch::markPoint()
{
    dotForMark.clear();
    for (size_t i = 0; i < dotInOrder.size(); i++){

        /// eachPoint存储待显示的对应点，内含6个int值，
        /// 依次为左点x、y，右点x、y，是否为已知点(1表示已知)，已知点编号
        vector<int> eachPoint;

        for (int j = 0; j < 4; j++){
            int value = dotInOrder[i][j];
            eachPoint.push_back(value);
        }

        bool known = false;//表示当前点i是否为已知

        if (scanSN%2 == 0){
            for (size_t p = 0; p < correspondPointEven.size(); p++){
                if (i == correspondPointEven[p].y){
                    eachPoint.push_back(1);
                    eachPoint.push_back(correspondPointEven[p].x);
                    known = true;
                    break;
                }
            }
        }
        else{
            for (size_t p = 0; p < correspondPointOdd.size(); p++){
                if (i == correspondPointOdd[p].y){
                    eachPoint.push_back(1);
                    eachPoint.push_back(correspondPointOdd[p].x);
                    known = true;
                    break;
                }
            }
        }
        if (!known){
            eachPoint.push_back(0);
            eachPoint.push_back(0);
        }
        dotForMark.push_back(eachPoint);
    }
}

////____________________________________________________________________////
/// \由findDot内部调用，用于对四点法找到的标记点进一步精确化
/// \param img 包含标记点的二值图像
/// \param vec 标记点坐标向量，每个vector<float>中依次为y值，x左值，x右值
/// \return 精细化后的标记点坐标
////____________________________________________________________________////
vector<Point2f> DotMatch::subPixel(Mat img, vector<vector<float>> vec)
{
    vector<Point2f> out;
    Point2f p;
    for (size_t i = 0; i < vec.size(); i++){
        p.x = (int)(vec[i][1] + vec[i][2])/2;
        p.y = (int)vec[i][0];

        ///Till now, the center coordinate has been calculated, the next step select an ROI around the center and perform a
        /// ellipse fitting using OpenCV
        Mat square = img(Rect(p.x-MIDUP,p.y-MIDUP,2*MIDUP,2*MIDUP));
        /*
        ///If the amount of black pixels in square beyond threshold, continue
        int black = 0;
        for (int r=0;r<square.rows;r++){
            uchar* pix = square.ptr<uchar>(r);
            for (int c=0;c<square.cols;c++){
                if (pix[c] == 0)
                    black++;
            }
        }
        if (black>450)//half of the square area
            continue;
        */
        vector<Point2f> centers;
        vector<vector<Point> > contours;
        findContours(square, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
        if (contours.size() > 4)
            continue;
        for(size_t i = 0; i < contours.size()-1; i=i+2){
            size_t count = contours[i].size();
            if( count < 16 )//如果该轮廓中所包含的点不足16个，则忽略
                continue;
            Mat pointsf;
            Mat(contours[i]).convertTo(pointsf, CV_32F);
            RotatedRect box = fitEllipse(pointsf);
            if( MAX(box.size.width, box.size.height) > MIN(box.size.width, box.size.height)*5 )
                continue;
            if(box.size.width <= 2 || box.size.height <= 2)
                continue;
            if(box.size.width > MIDUP || box.size.height > MIDUP)
                continue;
            box.center.x = box.center.x+p.x-MIDUP;
            box.center.y = box.center.y+p.y-MIDUP;
            centers.push_back(box.center);
        }
        if (!centers.empty()){
            p.x = centers[0].x;
            p.y = centers[0].y;
        }

        int xl = 0;
        int xr = 0;
        int yu = 0;
        int yd = 0;
        if (img.at<uchar>((int)p.y, (int)p.x) != 0){
            while (img.at<uchar>((int)p.y, ((int)p.x - xl)) > 0){
                xl++;
                if (xl > MIDUP || (p.x - xl) <= 0)
                    break;
            }
            while (img.at<uchar>((int)p.y, ((int)p.x + xr)) > 0){
                xr++;
                if (xr > MIDUP || (p.x + xr) >= img.cols)
                    break;
            }
            while (img.at<uchar>(((int)p.y + yu), (int)p.x) > 0){
                yu++;
                if (yu > MIDUP || (p.y + yu) >= img.rows)
                    break;
            }
            while (img.at<uchar>(((int)p.y - yd), (int)p.x) > 0){
                yd++;
                if (yd > MIDUP || (p.y - yd) <= 0)
                    break;
            }

            if (yd >= MIDUP || yu >= MIDUP || xr >= MIDUP || xl >= MIDUP){
                yd = 0;
                yu = 0;
                xr = 0;
                xl = 0;
                continue;
            }
            else{
                /*
                if (yu >= yd)
                    p.y = p.y + (yu-yd)/2;
                else
                    p.y = p.y - (yd-yu)/2;
                if (xl >= xr)
                    p.x  = p.x - (xl-xr)/2;
                else
                    p.x = p.x + (xr - xl)/2;
                */
                out.push_back(p);
            }
        }
    }
    return out;
}



////______________________________________________________////
///  大津法求解二值化阈值
////______________________________________________________////
int DotMatch::OSTU_Region(cv::Mat& image)
{
    assert(image.channels() == 1);
    int width = image.cols ;
    int height = image.rows ;
    int x = 0,y = 0;
    int pixelCount[256] = { 0 };
    float pixelPro[256] = { 0 };
    int i, j, pixelSum = width * height, threshold = 0;

    uchar* data = image.ptr<uchar>(0);

    //count every pixel number in whole image
    for(i = y; i < height; i++){
        for(j = x;j <width;j++)
            pixelCount[data[i * image.step + j]]++;
    }

    //count every pixel's radio in whole image pixel
    for(i = 0; i < 256; i++)
        pixelPro[i] = (float)(pixelCount[i]) / (float)(pixelSum);

    // segmentation of the foreground and background
    // To traversal grayscale [0,255],and calculates the variance maximum grayscale values ​​for the best threshold value
    float w0, w1, u0tmp, u1tmp, u0, u1, u,deltaTmp, deltaMax = 0;
    for(i = 0; i < 256; i++){
        w0 = w1 = u0tmp = u1tmp = u0 = u1 = u = deltaTmp = 0;

        for(j = 0; j < 256; j++){
            if(j <= i){ 	//background
                w0 += pixelPro[j];
                u0tmp += j * pixelPro[j];
            }
            else{ 		//foreground
                w1 += pixelPro[j];
                u1tmp += j * pixelPro[j];
            }
        }

        u0 = u0tmp / w0;
        u1 = u1tmp / w1;
        u = u0tmp + u1tmp;
        //Calculating the variance
        deltaTmp = w0 * (u0 - u)*(u0 - u) + w1 * (u1 - u)*(u1 - u);
        if(deltaTmp > deltaMax){
            deltaMax = deltaTmp;
            threshold = i;
        }
    }
    return threshold;
}

////_________________________________////
/// \brief DotMatch::isBelongTo
/// \param e
/// \param C
/// \return e是否属于集合C
////_________________________________////
bool DotMatch::isBelongTo(size_t e, vector<int> C)
{
    for (size_t i = 0; i < C.size(); i++){
        if (e == C[i])
            return true;
    }
    return false;
}


Triangle::Triangle(int Vertex_0, int Vertex_1, int Vertex_2, float distance_12, float distance_02, float distance_01)
{
    ver_0 = Vertex_0;
    ver_1 = Vertex_1;
    ver_2 = Vertex_2;
    dis_0 = distance_12;
    dis_1 = distance_02;
    dis_2 = distance_01;
}

bool Triangle::copmareTriangle(Triangle tri_known, Triangle tri_unknown, vector<Point2i> &corr, float &error)
{
    vector<float> dis_known;
    vector<float> dis_unknown;
    dis_known.push_back(tri_known.dis_0);
    dis_known.push_back(tri_known.dis_1);
    dis_known.push_back(tri_known.dis_2);
    dis_unknown.push_back(tri_unknown.dis_0);
    dis_unknown.push_back(tri_unknown.dis_1);
    dis_unknown.push_back(tri_unknown.dis_2);

    if (fabs(dis_unknown[0]-dis_unknown[1])<tolerance || fabs(dis_unknown[0]-dis_unknown[2])<tolerance ||
            fabs(dis_unknown[2]-dis_unknown[1])<tolerance){
        return false;
    }
    else{
        float e1=fabs(dis_known[0]-dis_unknown[0])+fabs(dis_known[1]-dis_unknown[1])+fabs(dis_known[2]-dis_unknown[2]);
        float e2=fabs(dis_known[0]-dis_unknown[1])+fabs(dis_known[1]-dis_unknown[0])+fabs(dis_known[2]-dis_unknown[2]);
        float e3=fabs(dis_known[0]-dis_unknown[0])+fabs(dis_known[1]-dis_unknown[2])+fabs(dis_known[2]-dis_unknown[1]);
        float e4=fabs(dis_known[0]-dis_unknown[2])+fabs(dis_known[1]-dis_unknown[0])+fabs(dis_known[2]-dis_unknown[1]);
        float e5=fabs(dis_known[0]-dis_unknown[2])+fabs(dis_known[1]-dis_unknown[1])+fabs(dis_known[2]-dis_unknown[0]);
        float e6=fabs(dis_known[0]-dis_unknown[1])+fabs(dis_known[1]-dis_unknown[2])+fabs(dis_known[2]-dis_unknown[0]);

        float minerror[]={e1,e2,e3,e4,e5,e6};
        float minvalue = minerror[0];
        int best = 0;
        for(size_t i=0;i<6;i++){
            if (minerror[i]<minvalue){
                minvalue = minerror[i];
                best = i;
            }
        }
        if (minvalue<tolerance){
            error=minvalue;
            switch (best){
            case 0:
                corr.push_back(Point2i(tri_known.ver_0,tri_unknown.ver_0));
                corr.push_back(Point2i(tri_known.ver_1,tri_unknown.ver_1));
                corr.push_back(Point2i(tri_known.ver_2,tri_unknown.ver_2));
                break;
            case 1:
                corr.push_back(Point2i(tri_known.ver_0,tri_unknown.ver_1));
                corr.push_back(Point2i(tri_known.ver_1,tri_unknown.ver_0));
                corr.push_back(Point2i(tri_known.ver_2,tri_unknown.ver_2));
                break;
            case 2:
                corr.push_back(Point2i(tri_known.ver_0,tri_unknown.ver_0));
                corr.push_back(Point2i(tri_known.ver_1,tri_unknown.ver_2));
                corr.push_back(Point2i(tri_known.ver_2,tri_unknown.ver_1));
                break;
            case 3:
                corr.push_back(Point2i(tri_known.ver_0,tri_unknown.ver_2));
                corr.push_back(Point2i(tri_known.ver_1,tri_unknown.ver_0));
                corr.push_back(Point2i(tri_known.ver_2,tri_unknown.ver_1));
                break;
            case 4:
                corr.push_back(Point2i(tri_known.ver_0,tri_unknown.ver_2));
                corr.push_back(Point2i(tri_known.ver_1,tri_unknown.ver_1));
                corr.push_back(Point2i(tri_known.ver_2,tri_unknown.ver_0));
                break;
            case 5:
                corr.push_back(Point2i(tri_known.ver_0,tri_unknown.ver_1));
                corr.push_back(Point2i(tri_known.ver_1,tri_unknown.ver_2));
                corr.push_back(Point2i(tri_known.ver_2,tri_unknown.ver_0));
                break;
            }
            return true;
        }
        else
            return false;
    }
}

float Triangle::calDistance(Point3f point_1, Point3f point_2)
{
    float xd = point_1.x - point_2.x;
    float yd = point_1.y - point_2.y;
    float zd = point_1.z - point_2.z;
    float disIJ = pow(xd, 2) + pow(yd, 2) + pow(zd, 2);
    disIJ = sqrtf(disIJ);//实际距离，单位mm
    return disIJ;
}


