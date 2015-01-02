#include "dotmatch.h"
#include "QMessageBox"

bool cameraLoaded = false;
int YDISTANCE = 15;//两相机标志点Y向距离小于该值认为是同一点
int EDGEUP = 9;//水平方向上标志点边缘黑色宽度上限
int EDGEDOWN = 3;
int MIDDOWN = 9;
int MIDUP = 29;
float tolerance = 40;//判断特征值是否相等时的误差

DotMatch::DotMatch(QObject *parent, QString projectPath) :
    QObject(parent)
{
    firstFind = true;//第一次查找标志点默认为基准点
    scanNo = 0;//表示扫描的次数，0表示第一次扫描

    rc = new Reconstruct;
    rc->calibFolder = new QString[2];
    rc->savePath_ = projectPath;
    rc->setCalibPath(projectPath +"/calib/left/", 0);
    rc->setCalibPath(projectPath +"/calib/right/", 1);
    rc->cameras = new  VirtualCamera[2];

    path = projectPath;
}

vector<vector<float>> DotMatch::findDot(Mat image ,int cam)//cam表示相机编号，0代表左，1代表右
{
    bwThreshold = OSTU_Region(image);
    Mat bimage = image >= bwThreshold;

    /****************四点匹配法*****************/
    vector<vector<float>> alltemp;
    vector<float> ptemp;
    for (int i = 0; i < bimage.rows; i++)
    {
        ptemp.push_back(i);
        for (int j = 0; j < bimage.cols - 1; j++)
        {
            if ((bimage.at<uchar>(i, j + 1) - bimage.at<uchar>(i, j)) != 0)//说明发生了状态跳变
            {
                ptemp.push_back(j);
            }
        }

        if (ptemp.size() > 4)
        {
            for (size_t p = 1; p <= ptemp.size() - 4; p++)
            {
                int d1 = ptemp[p+1] - ptemp[p];
                int d2 = ptemp[p+2] - ptemp[p+1];
                int d3 = ptemp[p+3] - ptemp[p+2];
                if (d1 > EDGEDOWN && d1 < EDGEUP && d2 >MIDDOWN && d2 < MIDUP && d3 > EDGEDOWN && d3 < EDGEUP)
                {
                    int length = alltemp.size();
                    int match = -1;
                    vector<float> localtemp;//包含3个元素，y值，x左值，x右值
                    localtemp.push_back(ptemp[0]);
                    localtemp.push_back(ptemp[p]);
                    localtemp.push_back(ptemp[p+3]);
                    if (length == 0)
                    {
                        alltemp.push_back(localtemp);
                    }
                    else
                    {
                        for (int q = 0; q < length; q++)
                        {
                            int dy = localtemp[0] - alltemp[q][0];
                            int dx = abs(localtemp[2] - alltemp[q][2]);
                            if (dy < 10 && dx < 4)
                            {
                                match = q;
                                break;
                            }
                        }
                        if (match >= 0)
                        {
                            int deltax = (alltemp[match][2] - alltemp[match][1]) - (localtemp[2] - localtemp[1]);
                            if (deltax < 0)
                            {
                                alltemp[match] = localtemp;
                            }
                            else if (deltax == 0)
                            {
                                alltemp[match][0] = (alltemp[match][0] + localtemp[0])/2;
                                alltemp[match][1] = localtemp[1];
                                alltemp[match][2] = localtemp[2];
                            }
                        }
                        else
                        {
                            alltemp.push_back(localtemp);
                        }
                    }
                }
            }
        }
        ptemp.clear();
    }

    vector<Point2f> out = subPixel(bimage, alltemp);//将初步得到的圆心坐标进一步精确
    vector<vector<float>> dotOutput;//用来存储得到的标志点坐标
    for (int i = out.size() - 1; i > -1; i--)
    {
        vector<float> point;
        point.push_back(out[i].x);
        point.push_back(out[i].y);
        //point.push_back(1);//输出为齐次坐标
        dotOutput.push_back(point);
    }

    /****************OpenCV检测******************
     vector<vector<Point> > contours;//二层向量，内层为轮廓点集，外层为向量集
    //Mat bsmooth = bimage;
    //medianBlur(bimage, bsmooth, 5);
    //bimage = bsmooth;
    //imshow("s",bimage);
    //cvWaitKey(1);

    findContours(bimage, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

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
    return dotOutput;
}



void DotMatch::matchDot(Mat limage,Mat rimage)
{
    dotInOrder.clear();

    if (scanNo%2 == 0)//整除2余数为零，判断为偶数
    {
        dotPositionEven.clear();
        correspondPointEven.clear();
    }
    else
    {
        dotPositionOdd.clear();
        correspondPointOdd.clear();
    }
    if (!cameraLoaded)
    {
        cameraLoaded =  rc->loadCameras();
        if (cameraLoaded)
        {
            rc->cameras[0].position = cv::Point3f(0,0,0);//findProjectorCenter();
            rc->cam2WorldSpace(rc->cameras[0], rc->cameras[0].position);
            rc->cameras[1].position = cv::Point3f(0,0,0);
            rc->cam2WorldSpace(rc->cameras[1], rc->cameras[1].position);
            fundMat = rc->cameras[0].fundamentalMatrix;
        }
    }
    ////找出左右图像中的标志点
    vector<vector<float>> leftDot = findDot(limage, 0);
    vector<vector<float>> rightDot = findDot(rimage, 1);

    ////判断两相机所摄标志点的对应关系
    int k = 0;//dotInOrder中现存点个数
    int nowMatch = 0;//防止rightDot中的同一点与leftDot中的不同点重复对应
    vector<int> alreadymatched;
    for(size_t i = 0; i < leftDot.size(); i++)
    {
        bool breakflag = false;
        for (size_t s = 0;s < alreadymatched.size();s++)
        {
            if (i == alreadymatched[s])
                breakflag = true;
        }
        if (breakflag)
            break;
        for(size_t j = nowMatch; j < rightDot.size();j++)
        {
            float lp[] = {leftDot[i][0], leftDot[i][1], 1.0};//齐次坐标
            float rp[] = {rightDot[j][0], rightDot[j][1], 1.0};
            cv::Mat ld(1, 3, CV_32F, lp);
            cv::Mat rd(1, 3, CV_32F, rp);

            cv::Mat ltor = rd * fundMat * ld.t();
            //cv::Mat rtol = ld * fundMat.t() * rd.t();
            float zlr = ltor.at<float>(0,0);
            //float zrl = rtol.at<float>(0,0);
            if(abs(zlr) > 1)//|| abs(zrl) > 0.9
                continue;
            if (fabs(leftDot[i][1] - rightDot[j][1]) > YDISTANCE)
                continue;

            /****判断当前点相对于上一点X坐标的正负，如正负不同，则不是对应点****/
            if (k != 0)
            {
                bool isbreak = false;
                for (int p = 0;p < k;p++)
                {
                    int checkLefft = leftDot[i][0] - dotInOrder[p][0];
                    int checkRight = rightDot[j][0] - dotInOrder[p][2];
                    if(checkLefft * checkRight <= 0)
                    {
                        isbreak = true;
                        break;
                    }
                }
                if (isbreak)
                {
                    nowMatch++;
                    break;
                }
            }

            vector<float> dot;
            dot.push_back(leftDot[i][0]);
            dot.push_back(leftDot[i][1]);
            dot.push_back(rightDot[j][0]);
            dot.push_back(rightDot[j][1]);
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
        return;

    /****如果是第一次扫描，所得到的全部标志点从零编号并保存****/
    cv::vector<cv::vector<float> > featureTemp;
    if (firstFind)
    {
        dotFeature.clear();
        featureTemp = calFeature(dotPositionEven);
        for (size_t p = 0;p < featureTemp.size(); p++)
        {
            dotFeature.push_back(featureTemp[p]);
            Point2i corr;
            corr.x = p;
            corr.y = p;
            correspondPointEven.push_back(corr);
        }
    }
    else
    {
        if (scanNo%2 == 0)
            featureTemp = calFeature(dotPositionEven);
        else
            featureTemp = calFeature(dotPositionOdd);

        success = dotClassify(featureTemp);
    }
    if (success)
    {
        firstFind = false;
        markPoint();
        scanNo++;
    }
    else
        return;
}


bool DotMatch::triangleCalculate()
{
    cv::Point2f leftDot;
    cv::Point2f rightDot;
    for (size_t i = 0; i < dotInOrder.size(); i++)
    {
        leftDot.x = dotInOrder[i][0];
        leftDot.y = dotInOrder[i][1];
        rightDot.x = dotInOrder[i][2];
        rightDot.y = dotInOrder[i][3];

        cv::Point3f cam1Point = Utilities::pixelToImageSpace(leftDot, rc->cameras[0]); //convert camera pixel to image space
        rc->cam2WorldSpace(rc->cameras[0], cam1Point);
        cv::Vec3f ray1Vector = (cv::Vec3f) (rc->cameras[0].position - cam1Point); //compute ray vector
        Utilities::normalize(ray1Vector);

        cv::Point3f cam2Point = Utilities::pixelToImageSpace(rightDot, rc->cameras[1]); //convert camera pixel to image space
        rc->cam2WorldSpace(rc->cameras[1], cam2Point);
        cv::Vec3f ray2Vector = (cv::Vec3f) (rc->cameras[1].position - cam2Point); //compute ray vector
        Utilities::normalize(ray2Vector);

        cv::Point3f interPoint;
        bool ok = Utilities::line_lineIntersection(rc->cameras[0].position, ray1Vector,rc->cameras[1].position, ray2Vector, interPoint);

        if(!ok)
            continue;

        if (scanNo%2 == 0)
            dotPositionEven.push_back(interPoint);
        else
            dotPositionOdd.push_back(interPoint);
    }

    if (scanNo%2 == 0)
    {
        if (dotPositionEven.size() < 3)
        {
            QMessageBox::warning(NULL, tr("Not enough Point"), tr("Point less than 3"));
            firstFind = true;
            return false;
        }
        else
            return true;
    }
    else
    {
        if (dotPositionOdd.size() < 3)
        {
            QMessageBox::warning(NULL, tr("Not enough Point"), tr("Point less than 3"));
            firstFind = true;
            return false;
        }
        else
            return true;
    }
}


//计算每点与其余点之间的距离平方作为该点特征值
//dotP为标记点空间三维绝对坐标(dotPositionOdd 或 dotPositionEven)
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


////根据当前扫描获得的特征值查找其中的已有点并计算变换矩阵
/// 并将新点加入dotFeature库
/// 注意这里的featureTemp中元素的序号与dotPositionOdd 或dotPositionEven是对应的

bool DotMatch::dotClassify(cv::vector<cv::vector<float> > featureTemp)
{
    int match = 0;//表示匹配的特征值个数
    int validpoint = 0;//表示找到的一致点个数，小于3则不能生成变换矩阵，则应重新获取
    size_t featureSize = dotFeature.size();
    int formermatch = 0;//记录match最大值
    int bestNo = 0;//达到最大匹配程度（match值最高）的 j
    bool matched = false;//表示featureTemp中的 i 点是否与dotFeature中的点匹配
    bool iscon;//判断dotFeature中的j点是否已经被匹配，如果是则continue

    vector<int> alreadymatched;
    for (size_t i = 0; i < featureTemp.size(); i++)
    {
        size_t j;
        formermatch = 0;
        bestNo = 0;
        matched = false;//初始假设featureTemp中的第i组值与dotfeature中的第j组值不匹配
        for (j = 0; j < featureSize; j++)
        {
            iscon = false;
            for (size_t s = 0; s < alreadymatched.size(); ++s)
            {
                if (j == alreadymatched[s])//dotFeature中已经发生匹配的点不再参与匹配
                    iscon = true;
            }
            if (iscon)
                continue;

            match = 0;//每次变换dotFeature中的待匹配序列号时，都要将之前的匹配数清零
            for (size_t p = 0; p < featureTemp[i].size(); p++)
            {
                for (size_t q = 0; q < dotFeature[j].size(); q++)
                {
                    float td = fabs(featureTemp[i][p] - dotFeature[j][q]);
                    if (td < tolerance)
                        match++;
                }
            }
            if (match > formermatch)
            {
                bestNo = j;
                formermatch = match;
            }
        }
        if (formermatch >= 2)//这里假设特征值没有相同的，若出现至少两个特征值匹配，说明第i点为原有点j
        {
            Point2i corr;
            corr.x = bestNo;//当前检测点在dotFeature中的序号
            corr.y = i;//当前检测点在dotInOrder中的序号
            alreadymatched.push_back(bestNo);
            if (scanNo%2 == 0)
            {
                correspondPointEven.push_back(corr);
            }
            else
            {
                correspondPointOdd.push_back(corr);
            }
            matched = true;
            validpoint++;
        }

        if (!matched)
        {
            dotFeatureTemp.push_back(featureTemp[i]);
        }
    }

    ////到这里已经找出了在本次扫描及上次扫描中的共有点，并存入dotPositionEven中
    ///  当dotPositionEven[i].size()=2时，说明第i点在两次扫描中都被找到，可用来计算变换矩阵
    if (validpoint > 3)
    {
        for (size_t i  = 0; i < dotFeatureTemp.size(); i++)
        {
            dotFeature.push_back(dotFeatureTemp[i]);
        }
        dotFeatureTemp.clear();
        calMatrix();
        return true;
    }
    else
    {
        QMessageBox::warning(NULL,tr("Fail"),tr("Alignment can't continue due to unenough point."));
        return false;
    }
}


////通过储存在correspondPointEven及correspondPointOdd中的一致点变换前后坐标计算变换矩阵
void DotMatch::calMatrix()
{
    if (dotPositionEven.size() == 0 || dotPositionOdd.size() == 0)
    {
        QMessageBox::warning(NULL, tr("Not Enough Data"), tr("Accuqaring point position data failed"));
        return;
    }
    cv::vector<Point3f> pFormer;
    cv::vector<Point3f> pLater;

    for (size_t i =0; i < correspondPointOdd.size(); i++)
    {
        for (size_t j = 0; j < correspondPointEven.size(); j++)
        {
            if (correspondPointOdd[i].x == correspondPointEven[j].x)
            {
                if (scanNo%2 == 0)
                {
                    pFormer.push_back(dotPositionOdd[correspondPointOdd[i].y]);
                    pLater.push_back(dotPositionEven[correspondPointEven[j].y]);
                }
                else
                {
                    pFormer.push_back(dotPositionEven[correspondPointEven[j].y]);
                    pLater.push_back(dotPositionOdd[correspondPointOdd[i].y]);
                }
            }
        }
    }

    /********Horn四元数法求解变换矩阵*********/

    std::vector<double> inpoints;
    for(size_t i = 0;i < pFormer.size();i++)
    {
        inpoints.push_back(pFormer[i].x);
        inpoints.push_back(pFormer[i].y);
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
    cv::Mat outMat(3,4,CV_64F,data);

    if (scanNo == 1)
    {
        transFormer = outMat;
    }
    else if (scanNo > 1)
    {
        cv::Range rangeR(0,3);
        cv::Range rangeT(3,4);
        cv::Mat formerMatR = transFormer(cv::Range::all(),rangeR);
        cv::Mat formerMatT = transFormer(cv::Range::all(),rangeT);
        transFormer(cv::Range::all(),rangeR) = formerMatR * outMat(cv::Range::all(),rangeR);
        transFormer(cv::Range::all(),rangeT) = formerMatT + outMat(cv::Range::all(),rangeT);
    }

    QString outMatPath = path + "/scan/transfer_mat" + QString::number(scanNo) + ".txt";
    Utilities::exportMat(outMatPath.toLocal8Bit(), transFormer);
}

void DotMatch::markPoint()
{
    dotForMark.clear();
    for (size_t i = 0; i < dotInOrder.size(); i++)
    {
        /*** eachPoint存储待显示的对应点，内含6个int值，分别为
         * 左点x、y，右点x、y，是否为已知点(1表示已知)，已知点编号
         * ***/
        vector<int> eachPoint;
        for (int j = 0; j < 4; j++)
        {
            int value = dotInOrder[i][j];
            eachPoint.push_back(value);
        }

        bool known = false;//表示当前点i是否为已知

        if (scanNo%2 == 0)
        {
            for (size_t p = 0; p < correspondPointEven.size(); p++)
            {
                if (i == correspondPointEven[p].y)
                {
                    eachPoint.push_back(1);
                    eachPoint.push_back(correspondPointEven[p].x);
                    known = true;
                    break;
                }
            }
        }
        else
        {
            for (size_t p = 0; p < correspondPointOdd.size(); p++)
            {
                if (i == correspondPointOdd[p].y)
                {
                    eachPoint.push_back(1);
                    eachPoint.push_back(correspondPointOdd[p].x);
                    known = true;
                    break;
                }
            }
        }
        if (!known)
        {
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
    for (size_t i = 0; i < vec.size(); i++)
    {
        p.x = (int)(vec[i][1] + vec[i][2])/2;
        p.y = (int)vec[i][0];
        int xl = 0;
        int xr = 0;
        int yu = 0;
        int yd = 0;
        if (img.at<uchar>(p.y, p.x) != 0)
        {
            while (img.at<uchar>(p.y, (p.x - xl)) > 0)
            {
                xl++;
                if (xl > MIDUP || (p.x - xl) <= 0)
                    break;
            }
            while (img.at<uchar>(p.y, (p.x + xr)) > 0)
            {
                xr++;
                if (xr > MIDUP || (p.x + xr) >= img.cols)
                    break;
            }
            while (img.at<uchar>((p.y + yu), p.x) > 0)
            {
                yu++;
                if (yu > MIDUP || (p.y + yu) >= img.rows)
                    break;
            }
            while (img.at<uchar>((p.y - yd), p.x) > 0)
            {
                yd++;
                if (yd > MIDUP || (p.y - yd) <= 0)
                    break;
            }

            if (yd >= MIDUP || yu >= MIDUP || xr >= MIDUP || xl >= MIDUP)
            {
                yd = 0;
                yu = 0;
                xr = 0;
                xl = 0;
                continue;
            }
            else
            {
                if (yu >= yd)
                {
                    p.y = p.y + (yu-yd)/2;
                }
                else
                {
                    p.y = p.y - (yd-yu)/2;
                }
                if (xl >= xr)
                {
                    p.x  = p.x - (xl-xr)/2;
                }
                else
                {
                    p.x = p.x + (xr - xl)/2;
                }
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



