#include "dotmatch.h"
#include "math.h"
#include "QMessageBox"

bool cameraLoaded = false;
int YDISTANCE = 5;//两相机标志点Y向距离小于该值认为是同一点

DotMatch::DotMatch(QObject *parent, QString projectPath) :
    QObject(parent)
{
    bwThreshold = 60;//二值化阈值
    firstFind = true;//第一次查找标志点默认为基准点
    scanNo = 0;//表示扫描的次数，0表示第一次扫描

    rc = new Reconstruct;
    rc->calibFolder = new QString[2];
    rc->setCalibPath(projectPath +"/calib/left/", 0);
    rc->setCalibPath(projectPath +"/calib/right/", 1);
    rc->cameras = new  VirtualCamera[2];

}

vector<vector<float>> DotMatch::findDot(Mat image ,int cam)//cam表示相机编号，0代表左，1代表右
{
    vector<vector<Point> > contours;//声明存储轮廓点的向量，本身是存储轮廓的向量
    Mat bimage = image >= bwThreshold;//这种生成二值图像的方法很简洁

	/*自创方法*/
	//bool initial;
	Point2i temp
	for (int i = 0; i < bimage.rows; i++)
	{
		for (int j = 0; j < bimage.cols - 1; j++)
		{
			//initial = bimage.at(i, 0);
			if (bimage.at(i, j + 1) - bimage.at(i, j) = 1)//说明发生了状态跳变（由白到黑）
			{
				temp.x = j;
				temp.y = i;
			}
			if (temp.size() != 0)
			{

			}
		}
	}
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
    return dotOutput;
}



void DotMatch::matchDot(Mat limage,Mat rimage)
{
    dotInOrder.clear();
    if (scanNo%2 == 0)//整除2余数为零，判断为偶数
        dotPositionEven.clear();
    else
        dotPositionOdd.clear();
    if (!cameraLoaded)
    {
        cameraLoaded =  rc->loadCameras();
        if (cameraLoaded)
        {
            rc->cameras[0].position = cv::Point3f(0,0,0);//findProjectorCenter();
            rc->cam2WorldSpace(rc->cameras[0], rc->cameras[0].position);
            rc->cameras[1].position = cv::Point3f(0,0,0);
            rc->cam2WorldSpace(rc->cameras[1], rc->cameras[1].position);
        }
    }
    ////找出左右图像中的标志点
    vector<vector<float>> leftDot = findDot(limage, 0);
    vector<vector<float>> rightDot = findDot(rimage, 1);

    ////判断两相机所摄标志点的对应关系
    int k = 0;//dotInOrder中现存点个数
    int nowMatch = 0;//防止rightDot中的同一点与leftDot中的不同点重复对应
    vector<float> yreference;//存储两个值，分别为0号基准点在左右相机中的y坐标，作为后续基准点的参考点
    yreference.clear();

    float deltaXL, deltaXR, thetaL, thetaR, pecent, judgeUp, judgeDown;

    for(int i = 0; i < leftDot.size(); i++)
    {
        for(int j = nowMatch; j < rightDot.size();j++)
        {

            if (yreference.size() == 0)//如果还没有参考点，即还没有有效点
            {
                if(leftDot[i][1] - rightDot[j][1] > YDISTANCE)
                    break;
                else if(leftDot[i][1] - rightDot[j][1] < -YDISTANCE)
                    continue;
            }
            else
            {
                /*
                deltaXL = (leftDot[i][0] - 640)/1280*25;
                deltaXR = (rightDot[j][0] - 640)/1280*25;
                thetaL = (80 - deltaXL)*3.14/360;
                thetaR = (80 + deltaXR)*3.14/360;
                pecent = sin(thetaL)/sin(thetaR);//pecent = yL/yR
                judgeUp = (leftDot[i][1] - yreference[0]) - (rightDot[j][1] - yreference[1]) * pecent;
                judgeDown = (leftDot[i][1] - yreference[0]) - (rightDot[j][1] - yreference[1]) * pecent;
                if(judgeUp > YDISTANCE)
                    break;
                else if(judgeDown  < -YDISTANCE)
                    continue;
               */
                ///*
                if(leftDot[i][1] - rightDot[j][1] > YDISTANCE)
                    break;
                else if(leftDot[i][1] - rightDot[j][1] < -YDISTANCE)
                    continue;
                //*/
            }

            if (k != 0)
            {
                int checkLefft = leftDot[i][0] - dotInOrder[k-1][0];
                int checkRight = rightDot[j][0] - dotInOrder[k-1][2];
                if(checkLefft * checkRight <= 0) //如果当前点相对于上一点X坐标正负不同，则不是对应点
                {
                    nowMatch++;
                    break;
                }
            }

            if (k == 0)
            {
                yreference.push_back(leftDot[i][1]);
                yreference.push_back(rightDot[j][1]);
            }

            vector<float> dot;
            dot.push_back(leftDot[i][0]);
            dot.push_back(leftDot[i][1]);
            dot.push_back(rightDot[j][0]);
            dot.push_back(rightDot[j][1]);
            dotInOrder.push_back(dot);//每个元素都是包含4个float的向量，依次为左x，y；右x，y
            k++;
            nowMatch++;
            break;
        }
    }

    triangleCalculate();//三角计算或许可以用opencv自带的triangulatePoints()函数

    if (scanNo%2 == 0)
    {
        if (dotPositionEven.size() < 3)
        {
            QMessageBox::warning(NULL, tr("Not enough Point"), tr("Point less than 3"));
            firstFind = true;
            return;
        }
    }
    else
    {
        if (dotPositionOdd.size() < 3)
        {
            QMessageBox::warning(NULL, tr("Not enough Point"), tr("Point less than 3"));
            firstFind = true;
            return;
        }
    }

    bool success = true;
    ////如果是第一次扫描，所得到的全部标志点从零编号并保存
    if (firstFind)
    {
        dotFeature.clear();
        correspondPointEven.clear();

        cv::vector<cv::vector<Point3f> > featureTemp = calFeature(dotPositionEven);
        for (int p = 0;p < featureTemp.size(); p++)
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
        cv::vector<cv::vector<Point3f> > featureTemp;

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



void DotMatch::triangleCalculate()
{
    cv::Point2f leftDot;
    cv::Point2f rightDot;
    for (int i = 0; i < dotInOrder.size(); i++)
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
}


//计算每点与其余点之间的三向距离作为该点特征值
//dotP为标记点空间三维绝对坐标
cv::vector<cv::vector<cv::Point3f>> DotMatch::calFeature(cv::vector<Point3f> dotP)
{
    cv::vector<cv::vector<cv::Point3f>> featureTemp;
    featureTemp.resize(dotP.size());
    for (int i = 0; i < dotP.size() - 1; i++)
    {
        for (int j = i + 1; j < dotP.size(); j++)
        {
            float xd = dotP[i].x - dotP[j].x;
            float yd = dotP[i].y - dotP[j].y;
            float zd = dotP[i].z - dotP[j].z;
            Point3f disIJ;
            disIJ.x = fabs(xd);
            disIJ.y = fabs(yd);
            disIJ.z = fabs(zd);

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

bool DotMatch::dotClassify(cv::vector<cv::vector<Point3f> > featureTemp)
{
    int match = 0;
    int validpoint = 0;//表示找到的一致点个数，小于3则不能生成变换矩阵，则应重新获取
    float tolerance = 1;
    int featureSize = dotFeature.size();
    for (int i = 0; i < featureTemp.size(); i++)
    {
        for (int j = 0; j < featureSize; j++)
        {
            for (int p = 0; p < featureTemp[i].size(); p++)
            {
                for (int q = 0; q < dotFeature[j].size(); q++)
                {
                    float tx = fabs(featureTemp[i][p].x - dotFeature[j][q].x);
                    float ty = fabs(featureTemp[i][p].y - dotFeature[j][q].y);
                    float tz = fabs(featureTemp[i][p].y - dotFeature[j][q].y);
                    if (tx < tolerance && ty < tolerance && tz < tolerance)
                        match++;
                }
            }
            if (match >= 2)//这里假设特征值没有相同的，若出现至少两个特征值匹配，说明第i点为原有点j
            {
                Point2i corr;
                corr.x = j;
                corr.y = i;
                if (scanNo%2 == 0)
                {
                    correspondPointEven.push_back(corr);
                }
                else
                {
                    correspondPointOdd.push_back(corr);
                }
                validpoint++;
            }
            else
            {
                dotFeatureTemp.push_back(featureTemp[i]);
            }
        }
    }

    ////到这里已经找出了在本次扫描及上次扫描中的共有点，并存入dotPositionEven中
    ///  当dotPositionEven[i].size()=2时，说明第i点在两次扫描中都被找到，可用来计算变换矩阵
    if (validpoint >=3)
    {
        for (int i  = 0; i < dotFeatureTemp.size(); i++)
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

}

void DotMatch::markPoint()
{
    dotForMark.clear();
    for (int i = 0; i < dotInOrder.size(); i++)
    {
        vector<int> eachPoint;
        for (int j = 0; j < 4; j++)
        {
            int value = dotInOrder[i][j];
            eachPoint.push_back(value);
        }
        if (scanNo%2 == 0)
        {
            for (int p = 0; p < correspondPointEven.size(); p++)
            {
                if (i == correspondPointEven[p].y)
                {
                    eachPoint.push_back(1);
                    eachPoint.push_back(correspondPointEven[p].x);
                }
                else
                    continue;
            }
        }
        else
        {
            for (int p = 0; p < correspondPointOdd.size(); p++)
            {
                if (i == correspondPointOdd[p].y)
                {
                    eachPoint.push_back(1);
                    eachPoint.push_back(correspondPointOdd[p].x);
                }
                else
                {
                    eachPoint.push_back(0);
                    eachPoint.push_back(0);
                }
            }
        }

        dotForMark.push_back(eachPoint);
    }
}


