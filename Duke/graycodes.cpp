#include "graycodes.h"

GrayCodes::GrayCodes(int scanW, int scanH, bool useepi)
{
    for (int i=0; i<GRAY_MAX_NUM; i++)
    {
        grayCodes[i]=NULL;//Initialize
    }
    height = scanH;
    width = scanW;
    imgsLoaded = false;
    useEpi = useepi;//注意该赋值必须放在calNumOfImgs之前
    calNumOfImgs();//calculate the number of the vertical and horizontal stripe images
    currentImgNum = 0;
}

GrayCodes::~GrayCodes()
{
}


void GrayCodes::calNumOfImgs()
{
    numOfColImgs = (int)ceil(log(double (width))/log(2.0));//ceil return the minimum integer that larger than the equation. When width=1280, numOfColImgs=11
    numOfRowImgs = (int)ceil(log(double (height))/log(2.0));//When height=800, numOfRowImgs=10
    if (useEpi)
        numOfImgs= 2 + 2*numOfColImgs;
    else
        numOfImgs= 2 + 2*numOfColImgs + 2*numOfRowImgs;//the first 2 images are all white and all black images
}

int GrayCodes::getNumOfImgs()
{
    return numOfImgs;
}

int GrayCodes::getNumOfRowBits()
{
    return numOfRowImgs;
}

int GrayCodes::getNumOfColBits()
{
    return numOfColImgs;
}

void GrayCodes::allocMemForImgs()
{
    for(int i=0; i<numOfImgs; i++){
        grayCodes[i] = cv::Mat(height,width,CV_8U);
    }
    imgsLoaded = true;
}

void GrayCodes::generateGrays()
{
    if(!imgsLoaded){
        allocMemForImgs();//allocate memory for images
    }
    grayCodes[0] = 255;
    grayCodes[1] = 0;
    int flag=0;
    for (int j=0; j<width; j++){
        int rem=0, num=j, prevRem=j%2;
        for (int k=0; k<numOfColImgs; k++){
            num=num/2;
            rem=num%2;
            if ((rem==0 && prevRem==1) || (rem==1 && prevRem==0)){
                flag=1;
            }
            else{
                flag= 0;
            }
            for (int i=0;i<height;i++){
                int pixel_color = flag*255;
                grayCodes[2*numOfColImgs-2*k].at<uchar>(i,j) = pixel_color;
                if(pixel_color > 0)
                    pixel_color = 0;
                else
                    pixel_color = 255;
                grayCodes[2*numOfColImgs-2*k+1].at<uchar>(i,j) = pixel_color;
            }
            prevRem=rem;
        }
    }

    if (!useEpi){//如果不使用极限校正，则也生成行条纹
        for (int i=0;i < height;i++){
            int rem=0, num=i, prevRem=i%2;
            for (int k=0; k<numOfRowImgs; k++){
                num=num/2;
                rem=num%2;
                if ((rem==0 && prevRem==1) || (rem==1 && prevRem==0)){
                    flag=1;
                }
                else{
                    flag= 0;
                }
                for (int j=0; j<width; j++){
                    int pixel_color = flag*255;
                    grayCodes[2*numOfRowImgs-2*k+2*numOfColImgs].at<uchar>(i,j) = pixel_color;
                    if(pixel_color > 0)
                        pixel_color = 0;
                    else
                        pixel_color = 255;
                    grayCodes[2*numOfRowImgs-2*k+2*numOfColImgs+1].at<uchar>(i, j) = pixel_color;
                }
                prevRem=rem;
            }
        }
    }
}

int GrayCodes::grayToDec(cv::vector<bool> gray)//convert a gray code sequence to a decimal number
{
    int dec = 0;
    bool tmp = gray[0];
    if(tmp)
        dec += (int) pow((float)2, int(gray.size() - 1));
    for(int i = 1; i < gray.size(); i++){
        tmp=Utilities::XOR(tmp,gray[i]);
        if(tmp)
            dec+= (int) pow((float)2,int (gray.size() - i - 1) );
    }
    return dec;
}
