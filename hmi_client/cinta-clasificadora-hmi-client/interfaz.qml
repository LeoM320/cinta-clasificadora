import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    width: 400
    height: 300
    color: "#2c3e50"

    Text {
        id: titulo
        text: "HMI Cinta Clasificadora"
        color: "white"
        font.pixelSize: 20
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 20
    }

    Button {
        text: "Arrancar Motor (Prueba C)"
        anchors.centerIn: parent
        onClicked: {
            // "backend" es el nombre que le daremos a tu clase C++ desde QML
            backend.iniciarMotorDesdeC()
        }
    }
}
