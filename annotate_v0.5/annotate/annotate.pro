QT       += core gui xml widgets

# the target
TARGET = annotation

BOOSTPATH = 'C:\\boost_1_50_0\\boost_1_50_0'

#TODO
#DEFINES += NO_OPENCV # remove the '#' in order not to use opencv
#OPENCV_ROOT = '/home/guanta/opencv/install240'
#$(HOME)
#OPENCV_ROOT = c:/OpenCV2.0
#OPENCV_SUFFIX = 200

# some project options
TEMPLATE = app

# source files
HEADERS += *.h \
    preparetypepage.h \
    prepareprocesspage.h \
    preparecheckpage.h \
    preparefinalpage.h \
    preparedatasetpage.h \
    preparephocpage.h \
    prepareprocesspage.h \
    preparefvpage.h \
    prepareaspage.h \
    preparecspage.h \
    processthread.h
SOURCES += *.cpp \
    preparetypepage.cpp \
    preparecheckpage.cpp \
    preparefinalpage.cpp \
    preparedatasetpage.cpp \
    preparephocpage.cpp \
    prepareprocesspage.cpp \
    preparefvpage.cpp \
    prepareaspage.cpp \
    preparecspage.cpp \
    processthread.cpp
FORMS	+= *.ui \
    recogResultPage.ui \
    preparetypepage.ui \
    preparecheckpage.ui \
    preparefinalpage.ui \
    preparedatasetpage.ui \
    preparephocpage.ui \
    prepareprocesspage.ui \
    preparefvpage.ui \
    prepareaspage.ui \
    preparecspage.ui
RESOURCES += icons.qrc

# lib/include dirs
INCLUDEPATH += src $${INCLUDEPATH}
INCLUDEPATH += $${BOOSTPATH}

# add opencv libraries and include path
#!contains(DEFINES, NO_OPENCV) {
#	LIBS += -lcv$${OPENCV_SUFFIX} -lcxcore$${OPENCV_SUFFIX} -L$${OPENCV_ROOT}/lib
#	INCLUDEPATH += $${OPENCV_ROOT}/include
#}

#unix: CONFIG += link_pkgconfig
#unix: PKGCONFIG += opencv
