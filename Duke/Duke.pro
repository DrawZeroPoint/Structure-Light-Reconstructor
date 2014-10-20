TEMPLATE = app

TARGET = Duke

QT += qml quick widgets

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
    virtualcamera.cpp

RESOURCES += \
    Resource/res.qrc

INCLUDEPATH += E:\opencv\build\include\
D:\VC\inc\

LIBS += -LE:\opencv\build\x86\vc10\lib\
-LD:\VC\lib\
-lopencv_core249\
-lopencv_highgui249\
-lopencv_imgproc249\
-lopencv_features2d249\
-lHVDAILT\
-lHVExtend\
-lHVUtil\
-lRaw2Rgb

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
    virtualcamera.h

OTHER_FILES += \
    Resource/cal.png \
    Resource/calib.png \
    Resource/camera.png \
    Resource/close.png \
    Resource/new.png \
    Resource/open.png \
    Resource/projector.png \
    Resource/reconstruct.png \
    Resource/save.png \
    Resource/scan.png \
    Resource/set.png \
    Resource/splash.png
