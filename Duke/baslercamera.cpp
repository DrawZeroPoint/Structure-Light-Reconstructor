#include "baslercamera.h"
#include <QMessageBox>

BaslerCamera::BaslerCamera(QObject *parent) :
    QObject(parent)
{
    // Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system.
    // is initialized during the lifetime of this object
    Pylon::PylonAutoInitTerm autoInitTerm;

}

void BaslerCamera::openCamera()
{
    CTlFactory& tlFactory = CTlFactory::GetInstance();

    // Get all attached devices and exit application if no device is found.
    DeviceInfoList_t devices;
    if ( tlFactory.EnumerateDevices(devices) == 0 )
    {
        QMessageBox::warning(NULL,tr("Basler Camera"),tr("Basler cameras were not found."));
    }

    // Create an array of instant cameras for the found devices and avoid exceeding a maximum number of devices.
    cameras.Initialize(min( devices.size(), c_maxCamerasToUse));

    // Create and attach all Pylon Devices.
    for ( size_t i = 0; i < cameras.GetSize(); ++i)
    {
        cameras[i].Attach( tlFactory.CreateDevice( devices[i]));
    }

    // Starts grabbing for all cameras starting with index 0. The grabbing
    // is started for one camera after the other. That's why the images of all
    // cameras are not taken at the same time.
    // However, a hardware trigger setup can be used to cause all cameras to grab images synchronously.
    // According to their default configuration, the cameras are
    // set up for free-running continuous acquisition.
    cameras.StartGrabbing();

    for( int i = 0; i < cameras.IsGrabbing(); ++i)
    {
        cameras.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException);

        // When the cameras in the array are created the camera context value
        // is set to the index of the camera in the array.
        // The camera context is a user settable value.
        // This value is attached to each grab result and can be used
        // to determine the camera that produced the grab result.
        // Now, the image data can be processed.
        pImageBuffer = (uint8_t *) ptrGrabResult->GetBuffer();
    }
}

void BaslerCamera::closeCamera()
{
    cameras.StopGrabbing();
    cameras.Close();
}
