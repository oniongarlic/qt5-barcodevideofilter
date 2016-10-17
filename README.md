[QAbstractVideoFilter](http://doc.qt.io/qt-5/qabstractvideofilter.html) based barcode filter, using zxing

This was thrown togeather from sources from another project. 
It builds. Untested otherwise at this time.

Remember to export to QtQuick:

```
#include "barcodevideofilter.h"
...
qmlRegisterType<BarcodeVideoFilter>("bar.code", 1,0, "BarcodeScanner");
```

See example.qml for an example on how to use it with a Camera.
