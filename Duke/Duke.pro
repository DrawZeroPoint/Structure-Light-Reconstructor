TEMPLATE = app

TARGET = Duke

QT += qml quick widgets opengl

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
    cameracalibration.cpp

RESOURCES += \
    Resource/res.qrc

INCLUDEPATH += E:\opencv\build\include\
D:\VC\inc\

LIBS += -LE:\opencv\build\x86\vc10\lib\
-LD:\VC\lib\
-lopencv_core249d\
-lopencv_highgui249d\
-lopencv_imgproc249d\
-lopencv_features2d249d\
-lopencv_calib3d249d\
-lHVDAILT\
-lHVExtend\
-lHVUtil\
-lRaw2Rgb\

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
    cameracalibration.h

FORMS += \
    mainwindow.ui \
    Set.ui

TRANSLATIONS += en.ts zh.ts

