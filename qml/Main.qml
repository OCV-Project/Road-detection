import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: rootWindow
    visible: true
    width: 1024
    height: 760 
    title: "Анализ дефектов дорожного покрытия"
    color: "#000000" 

    // Player status
    property string playState: "inactive" 
    property int frameCounter: 0

    // Status line
    footer: Rectangle {
        height: 30
        color: "#000000"
        border.color: "#1e1e2f"
        border.width: 1
        Text {
            id: statusBarText
            text: "Ожидание загрузки..."
            color: "#ffffff"
            font.pixelSize: 11
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 10
        }
    }

    Connections {
        target: videoBackend
        
        // End signal
        function onVideoEnded() {
            playState = "ended"
        }

        // Logs
        function onStatusMessage(msg) {
            statusBarText.text = msg
        }

        // Video render
        function onFrameReady(frame) {
            frameCounter++
            videoImage.source = "image://video/frame_" + frameCounter
        }
    }

    // Video diaog
    FileDialog {
        id: fileDialog
        title: "Выберите видео"
        nameFilters: ["Видео файлы (*.mp4 *.avi *.mkv)"]
        onAccepted: {
            frameCounter = 0
            playState = "playing"
            videoBackend.loadVideo(fileDialog.currentFile)
        }
    }

    // ONNX dialog
    FileDialog {
        id: modelDialog
        title: "Выберите файл модели YOLO"
        nameFilters: ["ONNX модели (*.onnx)"]
        onAccepted: videoBackend.loadModel(modelDialog.currentFile)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Top part (Player)
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#000000"

            Image {
                id: videoImage
                anchors.fill: parent
                anchors.margins: 10
                fillMode: Image.PreserveAspectFit
                source: "image://video/frame_0"
                cache: false
            }
        }

        // Bot part
        Rectangle {
            Layout.fillWidth: true
            height: 110
            color: "#000000"
            border.color: "#1e1e2f"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 20

                // Add AI model button
                Button {
                    id: btnModel
                    text: "Загрузить модель"
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 45
                    visible: playState === "inactive" || playState === "ended"
                    
                    background: Rectangle {
                        color: btnModel.hovered ? "white" : "black"
                        border.color: "white"
                        border.width: 1
                        radius: 8
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }
                    
                    contentItem: Text {
                        text: btnModel.text
                        color: btnModel.hovered ? "black" : "white"
                        font.bold: true
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: modelDialog.open()
                }

                // Add video button
                Button {
                    id: btnVideo
                    text: "Выбрать видео"
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 45
                    visible: playState === "inactive" || playState === "ended"
                    
                    background: Rectangle {
                        color: btnVideo.hovered ? "white" : "black"
                        border.color: "white"
                        border.width: 1
                        radius: 8
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }
                    
                    contentItem: Text {
                        text: btnVideo.text
                        color: btnVideo.hovered ? "black" : "white"
                        font.bold: true
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: fileDialog.open()
                }

                // Stop button
                Button {
                    id: btnStop
                    text: "Остановить"
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 45
                    visible: playState === "playing" || playState === "paused"
                    
                    background: Rectangle {
                        color: btnStop.hovered ? "white" : "black"
                        border.color: "white"
                        border.width: 1
                        radius: 8
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }
                    
                    contentItem: Text {
                        text: btnStop.text
                        color: btnStop.hovered ? "black" : "white"
                        font.bold: true
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        videoBackend.stopVideo()
                        playState = "inactive"
                    }
                }

                Item { Layout.fillWidth: true }

                // Playback button
                Button {
                    id: playbackBtn
                    // ⏸ , ▶, ↻ symbols
                    text: playState === "playing" ? "⏸" : (playState === "ended" ? "↻" : "▶")
                    Layout.preferredWidth: 55
                    Layout.preferredHeight: 55
                    visible: playState !== "inactive"

                    background: Rectangle {
                        color: playbackBtn.hovered ? "black" : "white"
                        border.color: playbackBtn.hovered ? "white" : "transparent"
                        border.width: 1
                        radius: 27.5
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    contentItem: Text {
                        text: playbackBtn.text
                        color: playbackBtn.hovered ? "white" : "black"
                        font.pixelSize: 22
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        if (playState === "playing") {
                            videoBackend.pauseVideo()
                            playState = "paused"
                        } else if (playState === "paused") {
                            videoBackend.resumeVideo()
                            playState = "playing"
                        } else if (playState === "ended") {
                            videoBackend.restartVideo()
                            playState = "playing"
                        }
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }
    }
}
