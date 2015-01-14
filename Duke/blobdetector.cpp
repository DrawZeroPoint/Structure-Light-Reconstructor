#include "blobdetector.h"

int thresholdStep = 10;
int minThreshold = 50;
int maxThreshold = 220;
int minRepeatability = 2;
int minDistBetweenBlobs = 10;

bool filterByColor = true;
int blobColor = 0;

bool filterByArea = true;
int minArea = 25;
int maxArea = 5000;

bool filterByCircularity = false;
float minCircularity = 0.8f;
float maxCircularity = std::numeric_limits<float>::max();

bool filterByInertia = true;
//minInertiaRatio = 0.6;
float minInertiaRatio = 0.1f;
float maxInertiaRatio = std::numeric_limits<float>::max();

bool filterByConvexity = true;
//minConvexity = 0.8;
float minConvexity = 0.95f;
float maxConvexity = std::numeric_limits<float>::max();

BlobDetector::BlobDetector()
{
}

void BlobDetector::findBlobs(const cv::Mat &binaryImage, vector<Point2d> &centers) const
{
    centers.clear();

    vector < vector<Point> > contours;
    Mat tmpBinaryImage = binaryImage.clone();
    findContours(tmpBinaryImage, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

    for (size_t contourIdx = 0; contourIdx < contours.size(); contourIdx++)
    {
        Point2d center;
        Moments moms = moments(Mat(contours[contourIdx]));
        if (filterByArea)
        {
            double area = moms.m00;
            if (area < minArea || area >= maxArea)
                continue;
        }

        if (filterByCircularity)
        {
            double area = moms.m00;
            double perimeter = arcLength(Mat(contours[contourIdx]), true);
            double ratio = 4 * CV_PI * area / (perimeter * perimeter);
            if (ratio < minCircularity || ratio >= maxCircularity)
                continue;
        }

        if (filterByInertia)
        {
            double denominator = sqrt(pow(2 * moms.mu11, 2) + pow(moms.mu20 - moms.mu02, 2));
            const double eps = 1e-2;
            double ratio;
            if (denominator > eps)
            {
                double cosmin = (moms.mu20 - moms.mu02) / denominator;
                double sinmin = 2 * moms.mu11 / denominator;
                double cosmax = -cosmin;
                double sinmax = -sinmin;

                double imin = 0.5 * (moms.mu20 + moms.mu02) - 0.5 * (moms.mu20 - moms.mu02) * cosmin - moms.mu11 * sinmin;
                double imax = 0.5 * (moms.mu20 + moms.mu02) - 0.5 * (moms.mu20 - moms.mu02) * cosmax - moms.mu11 * sinmax;
                ratio = imin / imax;
            }
            else
            {
                ratio = 1;
            }

            if (ratio < minInertiaRatio || ratio >= maxInertiaRatio)
                continue;
        }

        if (filterByConvexity)
        {
            vector < Point > hull;
            convexHull(Mat(contours[contourIdx]), hull);
            double area = contourArea(Mat(contours[contourIdx]));
            double hullArea = contourArea(Mat(hull));
            double ratio = area / hullArea;
            if (ratio < minConvexity || ratio >= maxConvexity)
                continue;
        }

        center = Point2d(moms.m10 / moms.m00, moms.m01 / moms.m00);

        if (filterByColor)
        {
            if (binaryImage.at<uchar> (cvRound(center.y), cvRound(center.x)) != blobColor)
                continue;
        }

        centers.push_back(center);
    }
}
