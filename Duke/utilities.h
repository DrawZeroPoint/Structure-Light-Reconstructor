#ifndef UTILITIES_H
#define UTILITIES_H

#include <opencv/cv.h>
#include "virtualcamera.h"
#include "graycodes.h"
#include <direct.h>
#include <math.h>

#define STRICT
#include <windows.h>
#include <algorithm>
using std::min;
using std::max;

class Utilities
{
public:
    Utilities(void);
    ~Utilities(void);
    static bool				XOR(bool val1, bool val2);
    static void				normalize(cv::Vec3f &vec);
    static void				normalize3dtable(double vec[3]);
    static void				pixelToImageSpace(double p[3], CvScalar fc, CvScalar cc);
    static cv::Point3f		pixelToImageSpace(cv::Point2f p, VirtualCamera cam);
    static cv::Point2f		undistortPoints( cv::Point2f p, VirtualCamera cam);
    static CvScalar			planeRayInter(CvScalar planeNormal,CvScalar planePoint, CvScalar rayVector, CvScalar rayPoint );
    static double			matGet2D(cv::Mat m, int row, int col);
    static double			matGet3D(cv::Mat m, int x, int y, int i);
    static cv::Vec3d		matGet3D(cv::Mat m, int x, int y);
    static void				matSet3D(cv::Mat m, int x, int y, cv::Vec3d);
    static void				matSet3D(cv::Mat m, int x, int y,int i, double val);
    static void				matSet2D(cv::Mat m, int row, int col, double val);
    static void				autoContrast(cv::Mat img_in, cv::Mat &img_out);
    static void				autoContrast(IplImage *img_in, IplImage *img_out);
    static void				exportMat(const char *path, cv::Mat m);
    static bool				line_lineIntersection(cv::Point3f p1, cv::Vec3f v1, cv::Point3f p2,cv::Vec3f v2,cv::Point3f &p);
    static int				accessMat(cv::Mat m, int x, int y, int i);
    static int				accessMat(cv::Mat m, int x, int y);
    static void				folderScan(const char *path);
};

#endif // UTILITIES_H
