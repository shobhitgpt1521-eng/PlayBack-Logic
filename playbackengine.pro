QT       += core gui
QT += widgets multimedia multimediawidgets opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 link_pkgconfig
PKGCONFIG += gstreamer-1.0 gstreamer-video-1.0 gstreamer-app-1.0 glib-2.0 gstreamer-gl-1.0
unix:!macx: LIBS += -lgstpbutils-1.0
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    FfmpegExporter.cpp \
    FileSelector.cpp \
    RangeSlider.cpp \
    StitchingPlayer.cpp \
    TimelineBuilder.cpp \
    TrimDialog.cpp \
    VideoPlayer.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    FfmpegExporter.h \
    FileSelector.h \
    RangeSlider.h \
    SegmentMeta.h \
    SegmentSlider.h \
    StitchingPlayer.h \
    TimelineBuilder.h \
    TrimDialog.h \
    VideoPlayer.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target



