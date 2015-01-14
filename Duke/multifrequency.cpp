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
    for (size_t i = 0; i < MULTI_MAX_NUM; i++)
    {
        MultiFreqImages[i] = cv::Mat(projH, projW, CV_32F);
    }
    for (size_t f = 0; f < 3; f++)
    {
        for (size_t phi = 0; phi < 4; phi++)
        {
            cv::Mat temp(projH,projW,CV_32F);
            for (size_t w = 0; w < projW; w++)
            {
                for (size_t h = 0; h < projH; h++)
                {
                    temp.at<float>(h,w) = 0.502+0.498*sin(float(PI*2*w*frequency[f]/projW+PI*phi/2));
                }
            }
            MultiFreqImages[4*f+phi] = temp;
        }
    }
}

cv::Mat MultiFrequency::getNextMultiFreq()
{
    testCount++;
    if (testCount == 13)
        testCount = 1;
    return MultiFreqImages[testCount-1];
}

int MultiFrequency::getNumOfImgs()
{
    return 12;
}
