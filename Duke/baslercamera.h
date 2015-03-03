#ifndef BASLERCAMERA_H
#define BASLERCAMERA_H

//Qt
#include <QObject>
//Pylon
#include <pylon/PylonIncludes.h>

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 10;

static const size_t c_maxCamerasToUse = 2;


class BaslerCamera : public QObject
{
    Q_OBJECT
public:
    BaslerCamera(QObject *parent = 0);

    const uint8_t *pImageBuffer;
    // This smart pointer will receive the grab result data.
    CGrabResultPtr ptrGrabResult;

    void openCamera();
    void closeCamera();

private:
    CInstantCameraArray cameras;

};

#endif // BASLERCAMERA_H
