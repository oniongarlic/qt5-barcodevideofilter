import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtMultimedia 5.5
import bar.code 1.0

Item {
    id: cameraItem
    focus: true;

    property bool scanOnly: true;
    property bool imageCapture: false
    property bool oneShot: false;

    property bool cameraSelectionEnabled: true

    property bool autoStart: false

    property string barcode;

    // Metadata
    property string metaSubject: ""

    // Flash, for simplicity we just use Off/Auto
    property bool flash: false

    signal barcodeFound(string data);
    signal imageCaptured(string path)

    // Emit after oneshot decoding
    signal decodeDone()

    Camera {
        id: camera
        deviceId: "/dev/video1" // XXX
        captureMode: scanOnly ? Camera.CaptureViewfinder : Camera.CaptureStillImage;
        onErrorStringChanged: console.debug("Error: "+errorString)
        onCameraStateChanged: {
            console.debug("State: "+cameraState)
            switch (cameraState) {
            case Camera.ActiveState:
                console.debug("DigitalZoom: "+maximumDigitalZoom)
                console.debug("OpticalZoom: "+maximumOpticalZoom)
                break;
            }
        }
        onCameraStatusChanged: console.debug("Status: "+cameraStatus)

        onDigitalZoomChanged: console.debug(digitalZoom)

        focus {
            focusMode: Camera.FocusContinuous
        }

        metaData.subject: metaSubject

        imageCapture {
            onImageCaptured: {
                console.debug("Image captured!")
                console.debug(camera.imageCapture.capturedImagePath)
                previewImage.source=preview
            }
            onCaptureFailed: {
                console.debug("Capture failed")
            }
            onImageSaved: {
                console.debug("Image saved: "+path)
                cameraItem.imageCaptured(path)
            }
        }

        flash.mode: cameraItem.flash ? Camera.FlashAuto : Camera.FlashOff

        Component.onCompleted: {
            console.debug("Camera is: "+deviceId)
            console.debug("Camera orientation is: "+orientation)

            // digitalZoom=1.0;
        }
    }

    BarcodeScanner {
        id: scanner
        //enabledFormats: BarcodeScanner.BarCodeFormat_2D | BarcodeScanner.BarCodeFormat_1D
        enabledFormats: BarcodeScanner.BarCodeFormat_1D
        rotate: camera.orientation!=0 ? true : false;
        onTagFound: {
            console.debug("TAG: "+tag);
            cameraItem.barcode=tag;
            barcodeFound(tag)
        }
        onDecodingStarted: {

        }
        onDecodingFinished: {
            if (succeeded && cameraItem.oneShot) {
                camera.stop();
                decodeDone();
            }
        }
    }

    ColumnLayout {
        spacing: 8
        anchors.fill: parent

        VideoOutput {
            id: videoOutput
            source: camera
            autoOrientation: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            Layout.minimumWidth: 320
            Layout.minimumHeight: 320
            fillMode: Image.PreserveAspectFit

            filters: imageCapture ? [] : [ scanner ]

            Image {
                id: previewImage
                fillMode: Image.PreserveAspectFit
                width: 128
                height: 128
                anchors.left: parent.left
                anchors.top: parent.top
                opacity: 0.75
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.debug("*** Camera click")
                    if (scanOnly)
                        return;
                    captureImage();
                }
                onPressAndHold: {
                    console.debug("*** Camera press'n'hold")
                    if (camera.lockStatus==Camera.Unlocked)
                        camera.searchAndLock();
                    else
                        camera.unlock();
                }
            }

            BusyIndicator {
                anchors.centerIn: parent
                visible: running
                running: camera.lockStatus==Camera.Searching
            }
        }

        Text {
            id: barcodeText
            Layout.fillWidth: true
            color: "red"
            text: cameraItem.barcode
            font.pointSize: 22
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Popup {
        id: cameraPopup
        modal: true

        width: parent.width/2
        height: parent.height/2

        ListView {
            id: cameraList
            anchors.fill: parent
            clip: true
            model: QtMultimedia.availableCameras
            delegate: Text {
                id: c
                text: modelData.displayName
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                font.pixelSize: 22
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        camera.deviceId=modelData.deviceId
                        cameraPopup.close();
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.minimumHeight: 32

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter

        Button
        {
            enabled: camera.cameraStatus==Camera.ActiveStatus && !scanOnly
            visible: !scanOnly
            Layout.fillHeight: true
            Layout.fillWidth: true
            text: "Capture"
            onClicked: captureImage();
        }

        Button {
            text: "Try again"
            Layout.fillHeight: true
            Layout.fillWidth: true
            visible: !oneShot && scanOnly
            enabled: camera.cameraStatus!=Camera.ActiveStatus
            onClicked: camera.start()
        }

        Button {
            text: "Cameras"
            visible: cameraSelectionEnabled
            onClicked: {
                cameraPopup.open();
            }
        }

        Button {
            text: cameraItem.flash ? "Auto" : "Off"
            onClicked: {
                cameraItem.flash=!cameraItem.flash
            }
        }

        Button {
            text: getFocusTitle(camera.lockStatus)
            enabled: camera.cameraStatus==Camera.ActiveStatus && Camera.lockStatus!=Camera.Searching
            onClicked: {
                if (camera.lockStatus==Camera.Unlocked)
                    camera.searchAndLock();
                else
                    camera.unlock();
            }
            Layout.fillHeight: true
            Layout.fillWidth: true

            function getFocusTitle(cls) {
                switch (cls) {
                case Camera.Unlocked:
                    return qsTr("Focus")
                case Camera.Searching:
                    return qsTr("Focusing")
                default:
                    return qsTr("Unlock")
                }
            }
        }
    }

    Component.onCompleted: {
        console.debug("Camera standby")
        if (autoStart)
            startCamera();
    }

    Component.onDestruction: {
        stopCamera();
    }

    function captureImage() {
        camera.imageCapture.capture();
    }

    function startCamera() {
        console.debug("Start camera")
        camera.start();
    }

    function stopCamera() {
        console.debug("Stop camera")
        camera.stop();
    }
}

