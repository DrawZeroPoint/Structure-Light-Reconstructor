TEMPLATE = app

TARGET = Duke

QT += quick widgets opengl

SOURCES += \
    graycodes.cpp \
    main.cpp \
    mainwindow.cpp \
    meshcreator.cpp \
    pointcloudimage.cpp \
    projector.cpp \
    reconstruct.cpp \
    set.cpp \
    utilities.cpp \
    virtualcamera.cpp \
    plyloader.cpp \
    glwidget.cpp \
    cameracalibration.cpp \
    dotmatch.cpp \
    multifrequency.cpp \
    blobdetector.cpp \
    dahengcamera.cpp \
    baslercamera.cpp \
    focusassistant.cpp

RESOURCES += \
    Resource/res.qrc

INCLUDEPATH += E:\opencv\build\include\
D:\VC\inc\
D:\mrpt\libs\base\include\
D:\mrpt\libs\scanmatching\include\
D:\mrpt\include\mrpt\mrpt-config\
D:\glm\
E:\freeglut-2.8.1\include\
D:\genicam\library\cpp\include\
D:\pylon\include\

LIBS += -LD:\mrpt\lib\
-LE:\freeglut-2.8.1\lib\x86\Debug\
-LE:\opencv\build\x86\vc10\lib\
-LD:\VC\lib\
-LD:\genicam\library\cpp\lib\win32_i86\
-LD:\pylon\lib\Win32\
-lopencv_core249d\
-lopencv_highgui249d\
-lopencv_imgproc249d\
-lopencv_features2d249d\
-lopencv_calib3d249d\
-lHVDAILT\
-lHVExtend\
-lHVUtil\
-lRaw2Rgb\
-llibmrpt-base122-dbg\
-llibmrpt-scanmatching122-dbg\
-lfreeglut\
-lGCBase_MD_VC100_v2_3\
-lPylonBase_MD_VC100\
-lPylonBootstrapper\
-lPylonGigE_MD_VC100_TL\
-lPylonUsb_MD_VC100_TL\
-lPylonUtility_MD_VC100\


# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    graycodes.h \
    mainwindow.h \
    meshcreator.h \
    pointcloudimage.h \
    projector.h \
    reconstruct.h \
    set.h \
    utilities.h \
    virtualcamera.h \
    plyloader.h \
    glwidget.h \
    cameracalibration.h \
    dotmatch.h \
    multifrequency.h \
    blobdetector.h \
    dahengcamera.h \
    baslercamera.h \
    focusassistant.h

FORMS += \
    mainwindow.ui \
    Set.ui \
    focusassistant.ui

TRANSLATIONS += en.ts zh.ts

