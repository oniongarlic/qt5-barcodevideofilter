#include "VideoFrameWrapper.h"

#include <QVideoFrame>
#include <QColor>
#include <QDebug>
#include <QDateTime>
#include <QtConcurrent/QtConcurrentMap>

#ifdef Q_PROCESSOR_ARM_V7
#include <arm_neon.h>
#endif

#include <cmath>

VideoFrameWrapper::VideoFrameWrapper() :
    m_width(0),
    m_height(0),
    m_data(nullptr)
{    
    //http://entropymine.com/imageworsener/grayscale/
    //Gray = 0.2126×Red + 0.7152×Green + 0.0722×Blue
    for (int i=0;i<256;i++) {
        m_r[i]=static_cast<uint8_t>(round(0.2126*i));
        m_g[i]=static_cast<uint8_t>(round(0.7152*i));
        m_b[i]=static_cast<uint8_t>(round(0.0722*i));
    }
}

inline uint8_t VideoFrameWrapper::grey(uint8_t r, uint8_t g, uint8_t b)
{
    return m_r[r]+m_g[g]+m_b[b];
}

VideoFrameWrapper::VideoFrameWrapper(const QVideoFrame &input)
{
    if (!input.isReadable()) {
        qWarning("QVideoFrame input is not readable!");
        return;
    }    

    frameToImage(input);
}

void VideoFrameWrapper::updateBuffer(const QVideoFrame &input)
{
    int w,h;

    w=input.width();
    h=input.height();

    if (w==m_width && h==m_height && m_data!=nullptr)
        return;

    m_width=w;
    m_height=h;

    if (m_data)
        delete m_data;

    m_data=new uchar[m_width*m_height];
}

VideoFrameWrapper::~VideoFrameWrapper()
{    
    delete m_data;
}

int VideoFrameWrapper::getWidth() const
{
    return m_width;
}

int VideoFrameWrapper::getHeight() const
{
    return m_height;
}

QVideoFrame::PixelFormat VideoFrameWrapper::getFormat() const
{
    return m_format;
}

unsigned char *VideoFrameWrapper::getData() const
{
    return m_data;
}

#if defined(Q_PROCESSOR_ARM_V7) // && defined(__ARM_NEON__)
inline void VideoFrameWrapper::frameBGR32toGray(const QVideoFrame &input, unsigned char *dest)
{
    uchar *src=(uchar *)input.bits();
    int n=input.width()*input.height();

    uint8x8_t rfac = vdup_n_u8 (77);
    uint8x8_t gfac = vdup_n_u8 (151);
    uint8x8_t bfac = vdup_n_u8 (28);
    n/=8;

    for (int i=0; i<n; i++)
    {
        uint16x8_t  temp;
        uint8x8_t result;
        uint8x8x3_t bgr = vld3_u8(src);

        // 0xBBGGRRff
        temp = vmull_u8 (bgr.val[0], bfac);
        temp = vmlal_u8 (temp, bgr.val[1], gfac);
        temp = vmlal_u8 (temp, bgr.val[2], rfac);

        result = vshrn_n_u16 (temp, 8);
        vst1_u8 (dest, result);
        src  += 8*4;
        dest += 8;
    }
}
#else
inline void VideoFrameWrapper::frameBGR32toGray(const QVideoFrame &input, unsigned char *dest)
{
    const uchar*bits=input.bits();
    const int w=input.width();
    const int h=input.height();
    for(int y=0;y<h;y++)
    {
        const int ys=y*w;
        for(int x=0;x<w;x++)
        {
            // 0xBBGGRRff
            const quint32 *src_packed = (const quint32 *)bits+ys+x;
            const quint32 c=src_packed[0];
            const quint8 b = (c & 0xFF000000) >> 24;
            const quint8 g = (c & 0x00FF0000) >> 16;
            const quint8 r = (c & 0x0000FF00) >> 8;

            dest[ys+x]=grey(r,g,b);
        }
    }
}
#endif

/**
 * @brief VideoFrameWrapper::frameToImage
 * @param input
 * @return
 *
 * Convert incoming QVideoFrame to 8-bit greyscale data and stored in m_data.
 * We assume that it is mapped and readable.
 *
 */
bool VideoFrameWrapper::frameToImage(const QVideoFrame &input)
{
    updateBuffer(input);

    m_format=input.pixelFormat();
    // qDebug() << "Frame format: " << input.pixelFormat();

    switch (m_format) {
    case QVideoFrame::Format_UYVY:
    case QVideoFrame::Format_RGB32:
    case QVideoFrame::Format_ARGB32:
    case QVideoFrame::Format_ARGB32_Premultiplied:
    case QVideoFrame::Format_RGB565:
    case QVideoFrame::Format_RGB555: {
        const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(input.pixelFormat());
        QImage image=QImage(input.bits(), input.width(), input.height(), input.bytesPerLine(), imageFormat);
        for(int y=0;y<input.height();y++)
        {
            for(int x=0;x<input.width();x++)
            {
                QRgb p=image.pixel(x,y);
                m_data[y*m_width+x]=grey(qRed(p), qGreen(p), qBlue(p));
            }
        }

        return true;
    }
    case QVideoFrame::Format_ABGR32:
    case QVideoFrame::Format_BGR32: {
        frameBGR32toGray(input, m_data);
        return true;
    }
    case QVideoFrame::Format_NV12:
    case QVideoFrame::Format_NV21:
    case QVideoFrame::Format_YUV420P: {
        const uchar* bits=input.bits();
#if 1
        memcpy(m_data, bits, input.height()*input.width());
#else
        for(int y=0;y<input.height();y++)
        {
            for(int x=0;x<input.width();x++)
            {
                const quint8 c=bits[y*input.width()+x];
                m_data[y*m_width+x]=c;
            }
        }
#endif
        return true;
    }        
    default:;
        qWarning() << "Unhandled VideoFrame format: " << input.pixelFormat();
    }
    return false;
}


