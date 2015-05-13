#include "pointcloudimage.h"

PointCloudImage::PointCloudImage(int imageW,int imageH, bool colorFlag)
{
    w = imageW;
    h = imageH;
    points = cv::Mat(h, w, CV_32FC3);
    if(colorFlag == true)
        color = cv::Mat(h, w, CV_8UC3,cv::Scalar(0));//由于相机所摄为灰度图像，因此将32FC3改为8UC3
    else
        color = NULL;
    numOfPointsForPixel =  cv::Mat(h, w, CV_8U, cv::Scalar(0));
}

PointCloudImage::~PointCloudImage(void)
{
}

bool PointCloudImage::setPoint(int i_w, int j_h, cv::Point3f point, cv::Vec3i colorgray)
{
    if(i_w>=w || j_h>=h)
        return false;
    setPoint(i_w,j_h,point);
    Utilities::matSet3D(color,i_w,j_h,colorgray);
    return true;
}

bool PointCloudImage::setPoint(int i_w, int j_h, cv::Point3f point)
{
    if(i_w>=w || j_h>=h)
        return false;

    Utilities::matSet3D(points, i_w, j_h, (cv::Vec3f)point);
    Utilities::matSet2D(numOfPointsForPixel, j_h, i_w, 1);

    return true;
}

bool PointCloudImage::getPoint(int i_w, int j_h, cv::Point3f &pointOut, cv::Vec3i &colorOut)
{
    if(i_w>=w || j_h>=h)
        return false;
    uchar num = numOfPointsForPixel.at<uchar>(j_h,i_w);
    if(num > 0){
        pointOut = (cv::Point3f) (Utilities::matGet3D(points,i_w,j_h) / (float) num);
        if(!color.empty())
            colorOut = (cv::Point3i) (Utilities::matGet3D(color,i_w,j_h) / (float) num);
        else
            colorOut = (cv::Point3i) (100,100,100);//如果color为空，则人为赋一个值
        return true;
    }
    else
        return false;
}

bool PointCloudImage::getPoint(int i_w, int j_h, cv::Point3f &pointOut)
{
    if(i_w>=w || j_h>=h)
        return false;
    uchar num = numOfPointsForPixel.at<uchar>(j_h,i_w);
    if(num > 0){
        pointOut = (cv::Point3f) (Utilities::matGet3D(points, i_w, j_h) / (float) num);
        return true;
    }
    else
        return false;
}

bool PointCloudImage::addPoint(int i_w, int j_h, cv::Point3f point, cv::Vec3i colorgray)
{
    if(i_w >= w || j_h >= h)//由于i_w为[0,w)取值,因此其不可能等于w，若出现等于，直接判定false
        return false;
    uchar num = numOfPointsForPixel.at<uchar>(j_h, i_w);
    if(num == 0)
        return setPoint(i_w, j_h, point, colorgray);
    addPoint(i_w, j_h, point);
    if(!color.empty()){
        cv::Vec3i c = Utilities::matGet3D(color, i_w, j_h);
        Utilities::matSet3D(color, i_w, j_h, colorgray + c);
    }
    else
        return false;
    return true;
}

bool PointCloudImage::addPoint(int i_w, int j_h, cv::Point3f point)
{
    if(i_w>=w || j_h>=h)
        return false;
    uchar num = numOfPointsForPixel.at<uchar>(j_h,i_w);
    if(num == 0)
        return setPoint(i_w,j_h,point);
    cv::Point3f p = Utilities::matGet3D(points,i_w,j_h);
    Utilities::matSet3D(points,i_w,j_h,(cv::Vec3f)(point + p));
    numOfPointsForPixel.at<uchar>(j_h,i_w) = num + 1;
    return true;
}

void PointCloudImage::exportXYZ(char path[], bool exportOffPixels, bool colorFlag)
{
    std::ofstream out;
    out.open(path);
    cv::Point3f p;
    cv::Vec3i c;
    for(int i = 0; i<w; i++){
        for(int j = 0; j<h; j++){
            uchar num = numOfPointsForPixel.at<uchar>(j,i);
            if(!exportOffPixels && num == 0)
                continue;
            getPoint(i,j,p,c);
            if(exportOffPixels && num == 0){
                p = cv::Point3f(0,0,0);
                c = cv::Point3i(0,0,0);
            }
            out<<p.x<<" "<<p.y<<" "<<p.z;
            if(colorFlag && !color.empty())
                out<<" "<<c[2]<<" "<<c[1]<<" "<<c[0]<<"\n";
            else
                out<<"\n";
        }
    }
    out.close();
}

void PointCloudImage::exportNumOfPointsPerPixelImg(char path[])
{
    cv::Mat projToCamRays(cvSize(w, h), CV_8U);
    float max=0;
    int maxX,maxY;
    for(int i=0; i<w; i++){
        for(int j=0; j<h; j++){
            uchar num = numOfPointsForPixel.at<uchar>(j,i);
            if(num > max){
                max = num;
                maxX=i;
                maxY=j;
            }
        }
    }

    for(int i=0; i<w; i++){
        for(int j=0; j<h; j++){
            uchar num = numOfPointsForPixel.at<uchar>(j,i);
            Utilities::matSet2D(projToCamRays, j, i, num/(float)(max * 255.0));
        }
    }
    cv::imwrite("reconstruction/projToCamRays.png",projToCamRays);
    std::ofstream out1;
    std::stringstream txt;
    txt<<path<<".txt";
    out1.open(txt.str().c_str() );
    out1<< "black color = 0\nwhite color = "<< max <<"\nmax Pixel: ("<<maxX<<","<<maxY<<")";
    out1.close();
}

int PointCloudImage::getWidth()
{
    return w;
}

int PointCloudImage::getHeight()
{
    return h;
}
