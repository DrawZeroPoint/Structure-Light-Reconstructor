#ifndef BLOBDETECTOR_H
#define BLOBDETECTOR_H

#include <opencv/cv.h>

using namespace cv;

class BlobDetector
{
public:
    BlobDetector();
    void findBlobs(const cv::Mat &binaryImage, vector<Point2d> &centers) const;
private:

};

#endif // BLOBDETECTOR_H
