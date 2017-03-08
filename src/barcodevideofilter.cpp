#include "barcodevideofilter.h"

#include <zxing/common/GlobalHistogramBinarizer.h>
#include <zxing/common/HybridBinarizer.h>
#include <zxing/common/GreyscaleLuminanceSource.h>
#include <zxing/Binarizer.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/DecodeHints.h>
#include <zxing/qrcode/ErrorCorrectionLevel.h>

#include <QCamera>
#include <QTextCodec>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>

// #define DEBUG_FILTER
// #define DEBUG_TIME

BarcodeVideoFilter::BarcodeVideoFilter(QAbstractVideoFilter *parent) :
    QAbstractVideoFilter(parent),
    m_rotate(false)
{
}

BarcodeVideoFilterRunnable::BarcodeVideoFilterRunnable(BarcodeVideoFilter *parent, uint filters, bool rotate)
    : m_parent(parent)
{
    m_decoder = new zxing::MultiFormatReader();
    m_hints = new zxing::DecodeHints();
    if (filters & BarCodeFormat_2D) {
        qDebug("2D");
        m_hints->addFormat(zxing::BarcodeFormat::QR_CODE);
        m_hints->addFormat(zxing::BarcodeFormat::DATA_MATRIX);
    }
    if (filters & BarCodeFormat_1D) {
        qDebug("1D");
        m_hints->addFormat(zxing::BarcodeFormat::CODE_39);
        m_hints->addFormat(zxing::BarcodeFormat::CODE_93);
        m_hints->addFormat(zxing::BarcodeFormat::CODE_128);
        m_hints->addFormat(zxing::BarcodeFormat::EAN_8);
        m_hints->addFormat(zxing::BarcodeFormat::EAN_13);
    }
    //m_hints->setTryHarder(true); // XXX Do we need this ?
    m_rotate=rotate;
    m_fhandler = new VideoFrameWrapper();
    m_tp=new QThreadPool();
}

QVideoFrame BarcodeVideoFilterRunnable::run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, QVideoFilterRunnable::RunFlags flags)
{
    Q_UNUSED(surfaceFormat)
    Q_UNUSED(flags)

    static bool onceonly=true;

    if (m_future.isRunning()) {        
        return *input;
    } else if (m_future.isFinished() && m_future.resultCount()>0) {
        bool r=m_future.result();        
        emit m_parent->decodingFinished(r);        
    } else if (m_future.isCanceled()) {
        qDebug("First!");
    }

    if (!input->isValid()) {
        qWarning("Frame is not valid?");
        return *input;
    }
#ifdef DEBUG_TIME
    qint64 s=QDateTime::currentMSecsSinceEpoch();
#endif
    if (!input->map(QAbstractVideoBuffer::ReadOnly)) {
        qWarning("Failed to map frame for reading!");
        return *input;
    }

    emit m_parent->decodingStarted();

    bool r=m_fhandler->frameToImage(*input);

    if (!r && onceonly) {
        emit m_parent->error("Unknown camera frame format"+m_fhandler->getFormat());
        onceonly=false;
    }

#ifdef DEBUG_TIME
    qDebug() << "F2I: " << QDateTime::currentMSecsSinceEpoch()-s;
#endif

    m_future=QtConcurrent::run(m_tp, this, &BarcodeVideoFilterRunnable::scanBarcode, m_fhandler);

    input->unmap();

    return *input;
}

bool BarcodeVideoFilterRunnable::scanBarcode(VideoFrameWrapper *ciw)
{
#ifdef DEBUG_TIME
    qint64 s=QDateTime::currentMSecsSinceEpoch();
#endif
    QString er;

#ifdef DEBUG_FILTER
    qDebug() << "scan-Thread" << QThread::currentThread();
#endif

    if (!ciw) {
        qWarning("Invalid frame");
        return false;
    }

    const int w=ciw->getWidth();
    const int h=ciw->getHeight();

    try {
        zxing::ArrayRef<char> arr = zxing::ArrayRef<char>((char *)ciw->getData(), w*h);
        zxing::Ref<zxing::LuminanceSource> source(new zxing::GreyscaleLuminanceSource(arr, w, h, 0, 0, w, h));
        //zxing::Ref<zxing::LuminanceSource> source(new zxing::GreyscaleLuminanceSource(arr, w, h, 0, h/3, w, h-h/3));
        zxing::HybridBinarizer *binz = new zxing::HybridBinarizer(source);

        Q_ASSERT(binz);

        zxing::Ref<zxing::Binarizer> bz(binz);
        zxing::BinaryBitmap *bb = new zxing::BinaryBitmap(bz);

        Q_ASSERT(bb);

        zxing::Ref<zxing::BinaryBitmap> bitmap(bb);

        Q_ASSERT(m_decoder);
        if (m_rotate) {            
            zxing::Ref<zxing::BinaryBitmap> rbitmap = bitmap->rotateCounterClockwise();
            res = m_decoder->decode(rbitmap, *m_hints);
        } else {
            res = m_decoder->decode(bitmap, *m_hints);
        }

#ifdef DEBUG_TIME
        qDebug() << "SBC+: " << QDateTime::currentMSecsSinceEpoch()-s;
#endif

        QString string = QString(res->getText()->getText().c_str());
        if (!string.isEmpty() && (string.length() > 0)) {
            int fmt = res->getBarcodeFormat().value;

            QString cs = QString::fromStdString(res->getCharSet());
            if (!cs.isEmpty()) {
                QTextCodec *codec = QTextCodec::codecForName(res->getCharSet().c_str());
                if (codec)
                    string = codec->toUnicode(res->getText()->getText().c_str());
                m_barcode=cs;
            }

#ifdef DEBUG_FILTER
            qDebug() << fmt << string;
#endif

            emit m_parent->tagFound(string);
            emit m_parent->tagFoundAdvanced(string, fmt, cs);

            return true;
        }
    }
    catch(zxing::ReaderException &e) {
#ifdef DEBUG_FILTER
        qDebug() << "RE" << e.what();        
#endif
        er=e.what();
    }
    catch(zxing::Exception &e)
    {
#ifdef DEBUG_FILTER
        qDebug() << "E" << e.what();
#endif
        er=e.what();
    }

#ifdef DEBUG_TIME
    qDebug() << "SBC-: " << QDateTime::currentMSecsSinceEpoch()-s;
#endif

    emit m_parent->error(er);

    return false;
}

QVideoFilterRunnable *BarcodeVideoFilter::createFilterRunnable()
{
    return new BarcodeVideoFilterRunnable(this, m_enabledFormats, m_rotate);
}

void BarcodeVideoFilter::setFormats(BarCodeFormat enabledFormats)
{
    if (m_enabledFormats == enabledFormats)
        return;

    m_enabledFormats = enabledFormats;    

    emit enabledFormatsChanged(enabledFormats);
}

QString BarcodeVideoFilter::formatToString(const int fmt)
{
    switch (fmt) {
      case zxing::BarcodeFormat::AZTEC:
          return "AZTEC";

      case zxing::BarcodeFormat::CODABAR:
          return "CODABAR";

      case zxing::BarcodeFormat::CODE_39:
          return "CODE_39";

      case zxing::BarcodeFormat::CODE_93:
          return "CODE_93";

      case zxing::BarcodeFormat::CODE_128:
          return "CODE_128";

      case zxing::BarcodeFormat::DATA_MATRIX:
          return "DATA_MATRIX";

      case zxing::BarcodeFormat::EAN_8:
          return "EAN_8";

      case zxing::BarcodeFormat::EAN_13:
          return "EAN_13";

      case zxing::BarcodeFormat::ITF:
          return "ITF";

      case zxing::BarcodeFormat::MAXICODE:
          return "MAXICODE";

      case zxing::BarcodeFormat::PDF_417:
          return "PDF_417";

      case zxing::BarcodeFormat::QR_CODE:
          return "QR_CODE";

      case zxing::BarcodeFormat::RSS_14:
          return "RSS_14";

      case zxing::BarcodeFormat::RSS_EXPANDED:
          return "RSS_EXPANDED";

      case zxing::BarcodeFormat::UPC_A:
          return "UPC_A";

      case zxing::BarcodeFormat::UPC_E:
          return "UPC_E";

      case zxing::BarcodeFormat::UPC_EAN_EXTENSION:
          return "UPC_EAN_EXTENSION";
    } // switch
    return QString();
}
