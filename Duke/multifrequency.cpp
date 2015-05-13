#include "multifrequency.h"

int frequency[3] = {70,64,59};
int testCount = 0;//用来做多频外插投影的测试

MultiFrequency::MultiFrequency(QObject *parent, int projwidth, int projheight) :
    QObject(parent)
{
    projW = projwidth;
    projH = projheight;
    imgsLoaded = false;
}

void MultiFrequency::generateMutiFreq()
{
    MultiFreqImages[0] = cv::Mat(projH,projW,CV_8U,cvScalar(255));
    MultiFreqImages[1] = cv::Mat(projH,projW,CV_8U,cvScalar(0));
    for (size_t i = 2; i < MULTI_NUM + 2; i++)//+2表示全白全黑图像
    {
        MultiFreqImages[i] = cv::Mat(projH, projW, CV_8U);
    }
    for (size_t f = 0; f < 3; f++){
        for (size_t phi = 0; phi < 4; phi++){
            cv::Mat temp(projH,projW,CV_8U);
            for (size_t w = 0; w < projW; w++){
                for (size_t h = 0; h < projH; h++){
                    temp.at<uchar>(h,w) = 135+79*cos(float(PI*2*w*frequency[f]/projW+PI*phi/2));
                }
            }
            MultiFreqImages[4*f + phi + 2] = temp;
        }
    }
}


int MultiFrequency::getNumOfImgs()
{
    return 14;
}
