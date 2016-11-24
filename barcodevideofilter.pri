INCLUDEPATH  += $$PWD/QZXing $$PWD/QZXing/zxing

HEADERS += $$PWD/src/VideoFrameWrapper.h \
    $$PWD/src/barcodevideofilter.h

SOURCES += $$PWD/src/VideoFrameWrapper.cpp \
   $$PWD/src/barcodevideofilter.cpp
    
include(qzxing/QZXing.pri)

