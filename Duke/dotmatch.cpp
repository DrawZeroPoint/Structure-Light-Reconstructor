#include "dotmatch.h"
#include "QMessageBox"

bool cameraLoaded = false;
int YDISTANCE = 15;//两相机标志点Y向距离小于该值认为是同一点
int EDGEUP = 10;//水平方向上标志点边缘黑色宽度上限
int EDGEDOWN = 0;
int MIDDOWN = 8;
int MIDUP = 40;
float EPIPOLARERROR = 0.1;//特征点匹配时极线误差上界
float tolerance = 2;//判断特征值是否相等时的误差

DotMatch::DotMatch(QObject *parent, QString projectPath, bool useManual) :
    QObject(parent)
{
    firstFind = true;//第一次查找标志点默认为基准点
    scanSN = 0;//表示扫描的次数，0表示第一次扫描

    rc = new Reconstruct;
    rc->calibFolder = new QString[2];
    rc->savePath_ = projectPath;
    rc->setCalibPath(projectPath +"/calib/left/", 0);
    rc->setCalibPath(projectPath +"/calib/right/", 1);
    rc->cameras = new  VirtualCamera[2];

    path = projectPath;
    useManualMatch = useManual;
}

DotMatch::~DotMatch()
{
}

vector<vector<float>> DotMatch::findDot(Mat image)
{
    vector<vector<float>> dotOutput;//用来存储得到的标志点坐标
    Mat bimage = Mat::zeros(image.size(), CV_8UC1);//二值化后的图像

#ifdef USE_ADAPTIVE_THRESHOLD
    adaptiveThreshold(image,bimage,255,ADAPTIVE_THRESH_MEAN_C,THRESH_BINARY,blocksize,60);
    //blocksize较大可以使处理效果趋向二值化，较小则趋向边缘提取；C值的作用在于抑制噪声
#else
    bwThreshold = OSTU_Region(image);
    bimage = image >= bwThreshold;
#endif

#ifdef DEBUG
    Mat cimage = Mat::zeros(bimage.size(), CV_8UC3);
    Mat dimage = Mat::zeros(bimage.size(), CV_8UC3);
    cvNamedWindow("Threshold",CV_WINDOW_NORMAL);
    imshow("Threshold",bimage);
    cvWaitKey();
#endif

#ifdef USE_FOUR_POINT
    /****************四点匹配法*****************/
    vector<vector<float>> alltemp;

    for (int i = 0; i < bimage.rows; i++)
    {
        vector<int> ptemp;
        ptemp.push_back(i);//表示对第i行进行处理
        for (int j = 0; j < bimage.cols - 1; j++)
        {
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
                            if (dy < 10 && dx < 4)
                            {
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

    vector<Point2f> out = subPixel(bimage, alltemp);//将初步得到的圆心坐标进一步精确

#ifdef DEBUG
    for (size_t i=0; i <out.size();i++)
    {
        circle(dimage,out[i],10,Scalar(0,255,0));
    }
    cvNamedWindow("Point Refined",CV_WINDOW_NORMAL);
    imshow("Point Refined",dimage);
    cvWaitKey();
#endif
    
    for (int i = out.size() - 1; i > -1; i--)
    {
        vector<float> point;
        point.push_back(out[i].x);
        point.push_back(out[i].y);
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
bool DotMatch::matchDot(Mat leftImage,Mat rightImage)
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
            rc->cameras[0].position = cv::Point3f(0,0,0);//findProjectorCenter();
            rc->cam2WorldSpace(rc->cameras[0], rc->cameras[0].position);
            rc->cameras[1].position = cv::Point3f(0,0,0);
            rc->cam2WorldSpace(rc->cameras[1], rc->cameras[1].position);
            fundMat = rc->cameras[0].fundamentalMatrix;
        }
    }
    ////找出左右图像中的标志点
    vector<vector<float>> dotLeft = findDot(leftImage);
    vector<vector<float>> dotRight = findDot(rightImage);

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
    for(size_t i = 0; i < dotLeft.size(); i++)
    {
        float error = 2*EPIPOLARERROR;
        int bestJ = -1;//与i点最为符合的j点
        for(size_t j = 0; j < dotRight.size();j++)
        {
            if (isBelongTo(j, rightmatched))
                continue;

            float pleft[] = {dotLeft[i][0], dotLeft[i][1], 1.0};//齐次坐标
            float pright[] = {dotRight[j][0], dotRight[j][1], 1.0};
            cv::Mat plmat(1, 3, CV_32F, pleft);
            cv::Mat prmat(1, 3, CV_32F, pright);

            cv::Mat ltor = prmat * fundMat * plmat.t();//cv::Mat rtol = ld * fundMat.t() * rd.t();
            float zlr = ltor.at<float>(0,0);//float zrl = rtol.at<float>(0,0);
            //if (fabs(dotLeft[i][1] - dotRight[j][1]) > YDISTANCE)
                //continue;
            if(fabs(zlr) < EPIPOLARERROR && fabs(zlr) < error){
                //判断当前点相对于上一点X坐标的正负，如正负不同，则不是对应点
                if (k != 0){
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
                        error = fabs(zlr);
                    }
                }
                else{
                    bestJ = j;
                    error = fabs(zlr);
                }
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
    cv::vector<Point3f> updatedDotPosition;//用来暂存更新后的标记点绝对坐标
    ///暂时认为新增标记点个数不大于20，设置点初值为z=1000，作为是否后来被赋值的判别条件
    updatedDotPosition.resize(dotFeature.size()+20, Point3f(0,0,1000));

    vector<int> pointKnown;//存储当前次扫描确认为已知的点

    /// (1)检出的已知点，其唯一编号可根据correspondPointCurrent确定，坐标值可根据dotPosition确定
    /// 将两项写入updatedDotPosition中的对应位置
    for (size_t i = 0; i < correspondPointCurrent.size(); i++){
        updatedDotPosition[correspondPointCurrent[i].x] = dotPositionCurrent[correspondPointCurrent[i].y];
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
                if (distance < tolerance && distance < formerError){
                    //初步认为k点可能为ID点，但还需要通过邻域检查
                    vector<vector<float>> currentFeature = calFeature(dotPositionCurrent);//计算当前次扫描各点的特征值
                    vector<int> neighborOfK = calNeighbor(currentFeature, k);//利用各点特征值得到k点临近点信息
                    vector<int> filteredNeighbor;//储存k点与所有已知点的远近信息
                    for (size_t z = 0; z < neighborOfK.size(); z++){
                        //将不是已知的点过滤掉
                        if (isBelongTo(neighborOfK[z], pointKnown))
                            filteredNeighbor.push_back(neighborOfK[z]);
                    }
                    if (checkNeighbor(neighborFeature[ID], filteredNeighbor)){
                        //如果邻域检查返回值为真，说明k点邻域情况与ID点相同，表明k点更可能是ID点
                        updatedDotPosition[ID] = dotPositionCurrent[k];
                        formerError = distance;
                        matchNum = k;
                        match = true;//说明疑似点k即原ID点
                        correspondPointCurrent.push_back(Point2i(ID,k));
                    }
                }
            }

            if (match){
                pointKnown.push_back(matchNum);
            }
            else{
                //说明i点与任意k点都不匹配，即i点在当前次扫描中未被看到
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
        //通过测试的点应为真正的全新点
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
    for (size_t i = 0; i < dotInOrder.size(); i++){
        dotLeft.x = dotInOrder[i][0];
        dotLeft.y = dotInOrder[i][1];
        dotRight.x = dotInOrder[i][2];
        dotRight.y = dotInOrder[i][3];

        cv::Point3f cam1Point = Utilities::pixelToImageSpace(dotLeft, rc->cameras[0]); //convert camera pixel to image space
        rc->cam2WorldSpace(rc->cameras[0], cam1Point);
        cv::Vec3f ray1Vector = (cv::Vec3f) (rc->cameras[0].position - cam1Point); //compute ray vector
        Utilities::normalize(ray1Vector);

        cv::Point3f cam2Point = Utilities::pixelToImageSpace(dotRight, rc->cameras[1]); //convert camera pixel to image space
        rc->cam2WorldSpace(rc->cameras[1], cam2Point);
        cv::Vec3f ray2Vector = (cv::Vec3f) (rc->cameras[1].position - cam2Point); //compute ray vector
        Utilities::normalize(ray2Vector);

        cv::Point3f interPoint;
        bool ok = Utilities::line_lineIntersection(rc->cameras[0].position, ray1Vector,rc->cameras[1].position, ray2Vector, interPoint);

        if(!ok){
            dotRemove.push_back(i);
            QMessageBox::warning(NULL, tr("Trangel Calculate"), tr("Point ") + QString::number(i) + tr(" calculation failed."));
            continue;
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
    for (size_t i = 0; i < dotP.size() - 1; i++)
    {
        for (size_t j = i + 1; j < dotP.size(); j++)
        {
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
/// 根据当前扫描获得的特征值查找其中的已有点并计算变换矩阵                         ////
/// 并将新点加入dotFeature库                                                                       ////
/// 注意featureTemp中元素的序号与dotPositionOdd 或dotPositionEven是对应的       ////
////__________________________________________________________________////
bool DotMatch::dotClassify(cv::vector<cv::vector<float> > featureTemp)
{
    vector<Point2i> correspondPoint;
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
                bool pass = true;
                //bool pass = checkNeighbor(neighborFeature[correspondPoint[c].x], neighborTemp);
                if (pass){
                    if (scanSN%2 == 0)
                        correspondPointEven.push_back(correspondPoint[c]);
                    else
                        correspondPointOdd.push_back(correspondPoint[c]);
                    validpoint++;
                }
            }
        }//2、遍历correspondPoint结束，标号c
    }//1、遍历featureTemp结束，标号i

    /// 至此featureTemp中所有被认为是已知的点都与dotFeature中的点一一对应，并被用来计算变换矩阵
    if (validpoint > 3){
        return true;
    }
    else{
        QMessageBox::warning(NULL,tr("Dot Classify"),tr("Alignment can't continue due to unenough point."));
        return true;//调试用：这里既是匹配点小于3，也为true，正常应为false
    }
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
/// \param referance 作为参考的序列
/// \param needcheck 被检查的序列
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
        int xl = 0;
        int xr = 0;
        int yu = 0;
        int yd = 0;
        if (img.at<uchar>(p.y, p.x) != 0){
            while (img.at<uchar>(p.y, (p.x - xl)) > 0){
                xl++;
                if (xl > MIDUP || (p.x - xl) <= 0)
                    break;
            }
            while (img.at<uchar>(p.y, (p.x + xr)) > 0){
                xr++;
                if (xr > MIDUP || (p.x + xr) >= img.cols)
                    break;
            }
            while (img.at<uchar>((p.y + yu), p.x) > 0){
                yu++;
                if (yu > MIDUP || (p.y + yu) >= img.rows)
                    break;
            }
            while (img.at<uchar>((p.y - yd), p.x) > 0){
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
                if (yu >= yd)
                    p.y = p.y + (yu-yd)/2;
                else
                    p.y = p.y - (yd-yu)/2;
                if (xl >= xr)
                    p.x  = p.x - (xl-xr)/2;
                else
                    p.x = p.x + (xr - xl)/2;
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
    for(i = y; i < height; i++)
    {
        for(j = x;j <width;j++)
        {
            pixelCount[data[i * image.step + j]]++;
        }
    }

    //count every pixel's radio in whole image pixel
    for(i = 0; i < 256; i++)
    {
        pixelPro[i] = (float)(pixelCount[i]) / (float)(pixelSum);
    }

    // segmentation of the foreground and background
    // To traversal grayscale [0,255],and calculates the variance maximum grayscale values ​​for the best threshold value
    float w0, w1, u0tmp, u1tmp, u0, u1, u,deltaTmp, deltaMax = 0;
    for(i = 0; i < 256; i++)
    {
        w0 = w1 = u0tmp = u1tmp = u0 = u1 = u = deltaTmp = 0;

        for(j = 0; j < 256; j++)
        {
            if(j <= i) 	//background
            {
                w0 += pixelPro[j];
                u0tmp += j * pixelPro[j];
            }
            else 		//foreground
            {
                w1 += pixelPro[j];
                u1tmp += j * pixelPro[j];
            }
        }

        u0 = u0tmp / w0;
        u1 = u1tmp / w1;
        u = u0tmp + u1tmp;
        //Calculating the variance
        deltaTmp = w0 * (u0 - u)*(u0 - u) + w1 * (u1 - u)*(u1 - u);
        if(deltaTmp > deltaMax)
        {
            deltaMax = deltaTmp;
            threshold = i;
        }
    }
    //return the best threshold;
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


