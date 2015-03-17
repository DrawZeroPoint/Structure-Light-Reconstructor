#include "dotmatch.h"
#include "QMessageBox"

bool cameraLoaded = false;
int YDISTANCE = 15;//两相机标志点Y向距离小于该值认为是同一点
int EDGEUP = 8;//水平方向上标志点边缘黑色宽度上限
int EDGEDOWN = 0;
int MIDDOWN = 8;
int MIDUP = 32;
float EPIPOLARERROR = 0.05;//特征点匹配时极线误差上界
float tolerance = 25;//判断特征值是否相等时的误差

DotMatch::DotMatch(QObject *parent, QString projectPath) :
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
}

vector<vector<float>> DotMatch::findDot(Mat image)
{
    vector<vector<float>> dotOutput;//用来存储得到的标志点坐标
    Mat bimage = Mat::zeros(image.size(), CV_8UC1);//二值化后的图像

#ifdef USE_ADAPTIVE_THRESHOLD
    adaptiveThreshold(image,bimage,255,ADAPTIVE_THRESH_MEAN_C,THRESH_BINARY,21,60);
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
        vector<float> ptemp;
        ptemp.push_back(i);//?
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
                    int pointCount = alltemp.size();//表示alltemp中已经存在的点数
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
    /****************OpenCV检测******************/
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
    int nowMatch = 0;//防止dotRight中的同一点与dotLeft中的不同点重复对应
    vector<int> alreadymatched;
    for(size_t i = 0; i < dotLeft.size(); i++)
    {
        bool breakflag = false;
        for (size_t s = 0;s < alreadymatched.size();s++)
        {
            if (i == alreadymatched[s])
                breakflag = true;
        }
        if (breakflag)
            break;
        for(size_t j = nowMatch; j < dotRight.size();j++)
        {
            float pleft[] = {dotLeft[i][0], dotLeft[i][1], 1.0};//齐次坐标
            float pright[] = {dotRight[j][0], dotRight[j][1], 1.0};
            cv::Mat plmat(1, 3, CV_32F, pleft);
            cv::Mat prmat(1, 3, CV_32F, pright);

            cv::Mat ltor = prmat * fundMat * plmat.t();
            //cv::Mat rtol = ld * fundMat.t() * rd.t();
            float zlr = ltor.at<float>(0,0);
            //float zrl = rtol.at<float>(0,0);
            if (fabs(dotLeft[i][1] - dotRight[j][1]) > YDISTANCE)
                continue;
            if(fabs(zlr) > EPIPOLARERROR)
                continue;

            /****判断当前点相对于上一点X坐标的正负，如正负不同，则不是对应点****/
            if (k != 0){
                bool isbreak = false;
                for (int p = 0;p < k;p++){
                    int checkLefft = dotLeft[i][0] - dotInOrder[p][0];
                    int checkRight = dotRight[j][0] - dotInOrder[p][2];
                    if(checkLefft * checkRight <= 0){
                        isbreak = true;
                        break;
                    }
                }
                if (isbreak){
                    nowMatch++;
                    break;
                }
            }

            vector<float> dot;
            dot.push_back(dotLeft[i][0]);
            dot.push_back(dotLeft[i][1]);
            dot.push_back(dotRight[j][0]);
            dot.push_back(dotRight[j][1]);

            dotInOrder.push_back(dot);//每个元素都是包含4个float的向量，依次为左x，y；右x，y

            alreadymatched.push_back(i);
            k++;
            nowMatch++;
            break;
        }
    }

    /****根据当前扫描次数的奇偶性将标记点三维坐标放入dotPositionOdd(Even)****/
    bool success = triangleCalculate();//三角计算或许可以用opencv自带的triangulatePoints()函数
    if (!success)
        return false;

    /****如果是第一次扫描，所得到的全部标志点从零编号并保存****/
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
            markPoint();
            return true;
        }
        else
            return false;
    }
}


void DotMatch::finishMatch()
{
    if (!firstFind){
        calMatrix();
        for (size_t i  = 0; i < dotFeatureTemp.size(); i++){
            dotFeature.push_back(dotFeatureTemp[i]);
        }
        ///利用添加了新点的dotFeature计算邻域信息
        neighborFeature.clear();
        for (size_t i = 0;i < dotFeature.size();i++){
            vector<int> neighborTemp = calNeighbor(dotFeature, i);
            neighborFeature.push_back(neighborTemp);
        }
        dotFeatureTemp.clear();
        markPoint();
    }
    else
        firstFind = false;
    scanSN++;
}


bool DotMatch::triangleCalculate()
{
    cv::Point2f dotLeft;
    cv::Point2f dotRight;
    vector<int> dotRemove;//不能成功三角计算的点需要去除，该向量存储待去除点的序号
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
            continue;
        }
        //能到达这里的dotInOrder[i]都是通过了三角计算的，因此由其计算得到的interPoint可以存入dotPosition
        if (scanSN%2 == 0)
            dotPositionEven.push_back(interPoint);
        else
            dotPositionOdd.push_back(interPoint);
    }

    if (scanSN%2 == 0){
        if (dotPositionEven.size() < 4){
            QMessageBox::warning(NULL, tr("Trangel Calculate"), tr("Point less than 4. Try adjusting the exposure."));
            //firstFind = true;这里完全重置似无必要
            return false;
        }
        else{
            for (size_t r=0;r<dotRemove.size();r++){
                dotInOrder.erase(dotInOrder.begin()+dotRemove[r]);
            }
            return true;
        }
    }
    else{
        if (dotPositionOdd.size() < 4){
            QMessageBox::warning(NULL, tr("Not enough Point"), tr("Point less than 4. Try adjusting the exposure."));
            //firstFind = true;
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

    dotFeatureTemp.clear();

    /***定义featureLib，保存featureTemp中各点所匹配的dotFeature中点的序号及匹配度***/
    vector<vector<Point2i>> featureLib;//Point2i x值表示dotFeature中点的序号，y值表示匹配度match，凡是匹配度大于2的都保存
    featureLib.resize(featureTemp.size());

    ///未知点遍历循环开始
    for (size_t i = 0; i < featureTemp.size(); i++){

        ///已知点遍历循环开始
        for (size_t j = 0; j < featureSize; j++){

            match = 0;//每次变换dotFeature中的待匹配序列号时，都要将之前的匹配数清零

            ///单个未知点特征值遍历循环开始
            for (size_t p = 0; p < featureTemp[i].size(); p++){

                ///单个已知点特征值遍历循环开始
                for (size_t q = 0; q < dotFeature[j].size(); q++){
                    float td = fabs(featureTemp[i][p] - dotFeature[j][q]);
                    if (td < tolerance)
                        match++;
                }///循环结束

            }///循环结束

            if (match >= 1)///这里若不做限制则是将featureTemp中每一点与dotFeature中每点的匹配度计算出来
                                ///匹配度小于1的点认为是全新点
            {
                featureLib[i].push_back(Point2i(j,match));
            }
        }///已知点遍历循环结束

    }///未知点循环结束

    ///至此featureLib中存储了未知点所对应的可能匹配的已知点及其匹配度，下一步
    /// 是遍历dotFeature各点，查找与各点具有最大匹配度的i

    for (size_t j = 0;j < featureSize;j++){
        int bestNum = -1;
        int formatch = 0;
        for (size_t i = 0;i < featureLib.size();i++){
            for (size_t s = 0;s < featureLib[i].size();s++){
                if (j == featureLib[i][s].x){
                    if (featureLib[i][s].y > formatch){
                        bestNum = i;
                        formatch = featureLib[i][s].y;
                    }
                }
            }
        }
        if (bestNum>=0&&formatch>2){
            bool needbreak = false;
            /*
            for (size_t e = 0;e < featureLib[bestNum].size();e++){
                if (featureLib[bestNum][e].x != j){
                    if (featureLib[bestNum][e].y > formatch){
                        needbreak = true;
                    }
                }
            }
*/
            if (!needbreak){
                correspondPoint.push_back(Point2i(j,bestNum));
            }
        }
    }

    /// 邻域检查，目的是分辨检出的各点邻域情况是否符合已知，如不符合，则排除该点
    for (size_t i = 0;i < featureTemp.size();i++){
        for (size_t c = 0;c < correspondPoint.size();c++){
            if (i==correspondPoint[c].y){
                std::sort(featureTemp[i].begin(),featureTemp[i].end());
                vector<int> neighborTemp;
                for (size_t e=0;e<featureTemp[i].size();e++){
                    for (size_t p=0;p<featureTemp.size();p++){
                        for (size_t q=0;q<featureTemp[p].size();q++){
                            if (featureTemp[i][e] == featureTemp[p][q] && i!=p){
                                //说明该组特征值是由i、p两点计算出来的
                                for (size_t r = 0;r < correspondPoint.size();r++){
                                    if(p == correspondPoint[r].y){
                                        neighborTemp.push_back(correspondPoint[r].x);
                                    }
                                }
                            }
                        }
                    }
                }
                bool pass = checkNeighbor(neighborFeature[correspondPoint[c].x], neighborTemp);
                if (pass){
                    if (scanSN%2 == 0)
                        correspondPointEven.push_back(correspondPoint[c]);
                    else
                        correspondPointOdd.push_back(correspondPoint[c]);
                    validpoint++;
                }
            }
        }
    }

    /// 至此featureTemp中所有被认为是已知的点都与dotFeature中的点一一对应，并被用来计算变换矩阵
    /// 而有一部分点可能由于匹配度较低，对这些点采取忽略处理，显示为未知点，但不加入dotFeature
    /// 而对于匹配度小于1的i点，即featureLib[i].size=0，认为是新点，将其特征值加入dotFeatureTemp
    if (validpoint > 3){
        for (size_t s = 0;s < featureLib.size();s++){
            if (featureLib[s].size()==0){
                dotFeatureTemp.push_back(featureTemp[s]);
            }
        }
        return true;
    }
    else{
        QMessageBox::warning(NULL,tr("Dot Classify"),tr("Alignment can't continue due to unenough point."));
        return false;
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

bool DotMatch::checkNeighbor(vector<int> nf, vector<int> nt)
{
    int error = 0;
    for (size_t i = 0;i < nt.size();i++){
        int flag = -1;
        for (size_t j = 0;j < nf.size();j++){
            if (nt[i] == nf[j]){
                flag = j;
            }
        }
        if (flag >= 0){
            for (size_t p = i;p < nt.size();p++){
                for (size_t q = 0;q <= flag;q++){
                    if (nt[p] == nf[q] && q != flag){
                        nf[q] = -1;//使其不再参与比较
                        error++;
                    }
                }
            }
        }
        else{
            continue;
        }
    }
    if (error <= 2)
        return true;//error = 2可能是因对象点误检造成的，不足以说明当前的位置错误
    else
        return false;
}

////通过储存在correspondPointEven及correspondPointOdd中的一致点变换前后坐标计算变换矩阵
void DotMatch::calMatrix()
{
    if (dotPositionEven.size() == 0 || dotPositionOdd.size() == 0){
        QMessageBox::warning(NULL, tr("Not Enough Data"), tr("Accuqaring point position data failed"));
        return;
    }
    cv::vector<Point3f> pFormer;
    cv::vector<Point3f> pLater;

    for (size_t i =0; i < correspondPointOdd.size(); i++){
        for (size_t j = 0; j < correspondPointEven.size(); j++){
            if (correspondPointOdd[i].x == correspondPointEven[j].x){
                if (scanSN%2 == 0){
                    pFormer.push_back(dotPositionOdd[correspondPointOdd[i].y]);
                    pLater.push_back(dotPositionEven[correspondPointEven[j].y]);
                }
                else{
                    pFormer.push_back(dotPositionEven[correspondPointEven[j].y]);
                    pLater.push_back(dotPositionOdd[correspondPointOdd[i].y]);
                }
            }
        }
    }

    /********Horn四元数法求解变换矩阵*********/

    std::vector<double> inpoints;
    for(size_t i = 0;i < pFormer.size();i++){
        inpoints.push_back(pFormer[i].x);//经过验证，前面的点为基准，后面的点为待移动点
        inpoints.push_back(pFormer[i].y);//求解出的矩阵是将后面的坐标对齐到前面
        inpoints.push_back(pFormer[i].z);
        inpoints.push_back(pLater[i].x);
        inpoints.push_back(pLater[i].y);
        inpoints.push_back(pLater[i].z);
    }
    std::vector<double> outQuaternion;
    outQuaternion.resize(7);//如果不首先确定大小，可能产生和指针有关的问题
    double flag = mrpt::scanmatching::HornMethod(inpoints,outQuaternion);
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
    cv::Mat outMat(3,4,CV_64FC1,data);

    cv::Range rangeR(0,3);
    cv::Range rangeT(3,4);

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
        Mat outR(3,3,CV_64FC1);//表示outMat的R部分
        Mat outT(3,1,CV_64FC1);//outMat的T部分

        outR = outMat(cv::Range::all(),rangeR).clone();//从outMat深拷贝
        outT = outMat(cv::Range::all(),rangeT).clone();

        ///在以下计算中，等号左值为初始化了的空矩阵，右值全部来自于局部块的深拷贝
        /// 因此不依赖于原矩阵的值，因此计算能够成立，而直接采用局部块进行计算是不行的！
        transR = matRotation * outR;//例如，当scanSN=2时，=R1*R2
        transT = matRotation * outT + matTransform;//=R1*T2+T1
        matRotation = transR;//更新数值为R1*R2
        matTransform = transT;//更新数值为//R1*T2+T1

        QString outTransPath = path + "/scan/transfer_mat" + QString::number(scanSN) + ".txt";
        Utilities::exportMatParts(outTransPath.toLocal8Bit(), transR, transT);
    }
}



void DotMatch::markPoint()
{
    dotForMark.clear();
    for (size_t i = 0; i < dotInOrder.size(); i++){
        /*** eachPoint存储待显示的对应点，内含6个int值，分别为
         * 左点x、y，右点x、y，是否为已知点(1表示已知)，已知点编号
         * ***/
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



/************大津法求解二值化阈值************/
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



