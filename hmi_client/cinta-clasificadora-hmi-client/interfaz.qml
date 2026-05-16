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

    // Contenedor para los controles del Heartbeat
    Column {
        anchors.centerIn: parent
        spacing: 20

        // Control de Encendido/Apagado
        Row {
            spacing: 15
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                text: "Heartbeat:"
                color: "white"
                font.pixelSize: 16
                anchors.verticalCenter: parent.verticalCenter
            }

            Switch {
                id: hbSwitch
                checked: true // Encendido por defecto (como en el micro)
                onCheckedChanged: enviarConfiguracion()
            }
        }

        // Control del Periodo (Frecuencia)
        Column {
            spacing: 5
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                text: "Periodo: " + hbSlider.value + " ms"
                color: "white"
                font.pixelSize: 16
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Slider {
                id: hbSlider
                from: 100    // Mínimo 100ms
                to: 5000     // Máximo 5 segundos
                stepSize: 100 // Saltos de 100ms
                value: 1000  // Por defecto 1 segundo
                onValueChanged: enviarConfiguracion()
            }
        }
    }

    // Función que envía los datos al backend en C++
    function enviarConfiguracion() {
        backend.configurarHeartbeat(hbSwitch.checked, hbSlider.value)
    }
}
