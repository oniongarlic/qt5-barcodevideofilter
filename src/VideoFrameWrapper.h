#ifndef CAMERAIMAGE_H
#define CAMERAIMAGE_H

#include <QImage>
#include <QString>
#include <QVideoFrame>

class VideoFrameWrapper
{
public:
    VideoFrameWrapper();
    VideoFrameWrapper(const QVideoFrame &input);
    ~VideoFrameWrapper();

    int getWidth() const;
    int getHeight() const;
    QVideoFrame::PixelFormat getFormat() const;

    unsigned char *getData() const;
  
    bool frameToImage(const QVideoFrame &input);        

private:
    void updateBuffer(const QVideoFrame &input);
    int m_width;
    int m_height;
    QVideoFrame::PixelFormat m_format;
    unsigned char *m_data;

    uint8_t m_r[256];
    uint8_t m_g[256];
    uint8_t m_b[256];
    uint8_t grey(uint8_t r, uint8_t g, uint8_t b);
    void frameBGR32toGray(const QVideoFrame &input, unsigned char *dest);
};

#endif //CAMERAIMAGE_H
