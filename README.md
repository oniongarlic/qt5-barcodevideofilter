[QAbstractVideoFilter](http://doc.qt.io/qt-5/qabstractvideofilter.html) based barcode filter, using zxing

This was thrown togeather from sources from another project. 
It builds. Static build tested and works.

Remember to export to QtQuick:

```
#include "barcodevideofilter.h"
...
qmlRegisterType<BarcodeVideoFilter>("bar.code", 1,0, "BarcodeScanner");
```

See example.qml for an example on how to use it with a Camera.

## Static build

You can include the code statically in your project, for example as a 
git submodule. Use your own fork if you prefer that.

```
git submodule add https://github.com/oniongarlic/qt5-barcodevideofilter
```

Then include barcodevideofilter.pri in your project file:
```
include(qt5-barcodevideofilter/barcodevideofilter.pri)
```
