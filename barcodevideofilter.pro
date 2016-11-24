TEMPLATE = lib

QT += qml quick multimedia

INCLUDEPATH  += QZXing QZXing/zxing

HEADERS += src/VideoFrameWrapper.h \
    src/barcodevideofilter.h

SOURCES += src/VideoFrameWrapper.cpp \
    src/barcodevideofilter.cpp

include(qzxing/QZXing.pri)
