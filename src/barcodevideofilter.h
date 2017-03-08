#ifndef BARCODEVIDEOPROBE_H
#define BARCODEVIDEOPROBE_H

#include <QObject>
#include <QVideoFrame>
#include <QAbstractVideoFilter>
#include <QFuture>
#include <QThreadPool>

#include <zxing/common/GlobalHistogramBinarizer.h>
#include <zxing/common/HybridBinarizer.h>
#include <zxing/Binarizer.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/DecodeHints.h>
#include <zxing/qrcode/ErrorCorrectionLevel.h>
#include <zxing/zxing/LuminanceSource.h>

#include "VideoFrameWrapper.h"

class BarcodeVideoFilter;

class BarcodeVideoFilterRunnable : public QVideoFilterRunnable
{
public:
    explicit BarcodeVideoFilterRunnable(BarcodeVideoFilter *parent, uint filters, bool rotate=false);
    QVideoFrame run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags);

    enum FilterBarCodeFormats {
        BarCodeFormat_1D=0x01,
        BarCodeFormat_2D=0x02
    };

private:
    bool scanBarcode(VideoFrameWrapper *ciw);
    BarcodeVideoFilter *m_parent;
    zxing::MultiFormatReader *m_decoder;
    zxing::DecodeHints *m_hints;
    zxing::Ref<zxing::Result> res;
    QFuture<bool> m_future;
    QString m_barcode;
    bool m_rotate;
    VideoFrameWrapper *m_fhandler;
    QThreadPool *m_tp;
};

class BarcodeVideoFilter : public QAbstractVideoFilter
{
    Q_OBJECT
    Q_FLAGS(BarCodeFormat)
    Q_ENUMS(BarCodeFormats)
    Q_PROPERTY(BarCodeFormat enabledFormats READ getEnabledFormats WRITE setFormats NOTIFY enabledFormatsChanged)
    Q_PROPERTY(bool rotate READ rotate WRITE setRotate NOTIFY rotateChanged)

public:
    explicit BarcodeVideoFilter(QAbstractVideoFilter *parent = 0);
    QVideoFilterRunnable* createFilterRunnable();

    enum BarCodeFormats {
        BarCodeFormat_1D=0x01,
        BarCodeFormat_2D=0x02
    };
    Q_DECLARE_FLAGS(BarCodeFormat, BarCodeFormats)

    Q_INVOKABLE QString formatToString(const int fmt);

    uint getEnabledFormats() const
    {
        return m_enabledFormats;
    }

    bool rotate() const
    {
        return m_rotate;
    }

public slots:
    void setFormats(BarcodeVideoFilter::BarCodeFormat enabledFormats);

    void setRotate(bool rotate)
    {
        if (m_rotate == rotate)
            return;

        m_rotate = rotate;
        emit rotateChanged(rotate);
    }

signals:
    void finished(QPointF result);
    void decodingStarted();
    void decodingFinished(bool succeeded);
    void tagFound(QString tag);
    void tagFoundAdvanced(QString tag, int format, QString charSet);
    void error(QString msg);

    void unknownFrameFormat(int format, int width, int height);

    void enabledFormatsChanged(uint enabledFormats);

    void rotateChanged(bool rotate);

private:
    BarCodeFormat m_enabledFormats;
    bool m_rotate;    
};


#endif // BARCODEVIDEOPROBE_H
