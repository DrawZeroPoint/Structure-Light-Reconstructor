#include "graycodes.h"

GrayCodes::GrayCodes(int projW, int projH)
{
    for (int i=0; i<GRAY_MAX_NUM; i++)
    {
        grayCodes[i]=NULL;//Initialize
    }
    height = projH;
    width = projW;
    imgsLoaded = false;
    calNumOfImgs();//calculate the number of the vertical and horizontal stripe images
    currentImgNum = 0;
}

GrayCodes::~GrayCodes()
{
    //if(imgsLoaded)
        //unload();
}


void GrayCodes::calNumOfImgs()
{
    numOfColImgs = (int)ceil(log(double (width))/log(2.0));//ceil return the minimum integer that larger than the equation. When width=1280, numOfColImgs=11
    numOfRowImgs = (int)ceil(log(double (height))/log(2.0));//When height=800, numOfRowImgs=10
    numOfImgs= 2 + 2*numOfColImgs + 2*numOfRowImgs;//the first 2 images are all white and all black images
}

/*
void GrayCodes::unload()
{
    for(int i=0; i < numOfImgs;i++ ){
        if(grayCodes[i]){
            cvReleaseImage(&grayCodes[i]);
            grayCodes[i]=NULL;
        }
    }
    imgsLoaded=false;
}
*/


cv::Mat GrayCodes::getNextImg()
{
    if(currentImgNum<numOfImgs){
        currentImgNum++;
        return grayCodes[currentImgNum - 1];//currentImgNum是从0开始的，++操作后变为1，所以要-1
    }
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
        //grayCodes[i]=cvCreateImage(cvSize(width,height),IPL_DEPTH_8U,1);
        grayCodes[i] = cv::Mat(height,width,CV_8U);
    }
    imgsLoaded=true;
}

void GrayCodes::generateGrays()
{
    if(!imgsLoaded){
        allocMemForImgs();//allocate memory for images
    }
    for (int j=0; j<width; j++) //generate all white and all black images
    {
        for (int i=0;i<height;i++){
            //CvScalar pixel_color;
            //pixel_color.val[0] = 255;
            //cvSet2D(grayCodes[0], i, j, pixel_color);
            //pixel_color.val[0] = 0;
            //cvSet2D(grayCodes[1], i, j, pixel_color);
            grayCodes[0].at<uchar>(i,j) = 255;
            grayCodes[1].at<uchar>(i,j) = 0;
        }
    }
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
                /*
                CvScalar pixel_color;//cvscalar is an array that contain 4 double type elements
                pixel_color.val[0] = float(flag*255);
                cvSet2D(grayCodes[2*numOfColImgs-2*k], i, j, pixel_color);
                if(pixel_color.val[0]>0)
                    pixel_color.val[0]=0;
                else
                    pixel_color.val[0]=255;
                cvSet2D(grayCodes[2*numOfColImgs-2*k+1], i, j, pixel_color);
                */
                int pixel_color = flag*255;
                //cvSet2D(grayCodes[2*numOfColImgs-2*k], i, j, pixel_color);
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

    for (int i=0;i<height;i++){
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

/*
void GrayCodes::save()
{
    for(int i = 0; i < numOfImgs; i++){
        std::stringstream path;
        if(i+1<10)
            path<<"0";
        path<< i+1 <<".png";
        cvSaveImage(path.str().c_str(),grayCodes[i]);
    }
}
*/


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
