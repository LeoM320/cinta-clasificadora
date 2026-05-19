import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 450
    height: 550
    color: "#2b2b2b" // Fondo oscuro neutro

    // ==========================================
    // ESCUCHA DE EVENTOS DESDE C++
    // ==========================================
    Connections {
        target: backend

        function onDistanceUpdated(cm) {
            lblDistancia.text = cm + " cm"
        }

        function onIrStatesUpdated(ir0, ir1, ir2, ir3) {
            lblIR.text = "IR0: " + (ir0 ? "1" : "0") + " | " +
                         "IR1: " + (ir1 ? "1" : "0") + " | " +
                         "IR2: " + (ir2 ? "1" : "0") + " | " +
                         "IR3: " + (ir3 ? "1" : "0")
        }

        function onConnectionStatusChanged(connected) {
            if (!connected) {
                lblDistancia.text = "-- cm"
                lblIR.text = "Desconectado"
            }
        }
    }

    // ==========================================
    // INTERFAZ GRÁFICA (Layout Vertical)
    // ==========================================
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        Label {
            text: "PANEL DE PRUEBAS MANUAL"
            font.pixelSize: 18
            font.bold: true
            color: "#45a29e"
            Layout.alignment: Qt.AlignHCenter
        }

        // ---------- 1. TELEMETRÍA (Lectura Manual) ----------
        GroupBox {
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                // Título manual para evitar errores de estilo en el GroupBox
                Label {
                    text: "SENSORES"
                    color: "white"
                    font.bold: true
                }

                // Fila Ultrasónico
                RowLayout {
                    Button {
                        text: "Pedir Distancia"
                        onClicked: backend.requestDistance()
                    }
                    Label {
                        id: lblDistancia
                        text: "-- cm"
                        color: "#66fcf1"
                        font.pixelSize: 16
                        font.bold: true
                        Layout.leftMargin: 10
                    }
                }

                // Fila Infrarrojos
                RowLayout {
                    Button {
                        text: "Pedir Infrarrojos"
                        onClicked: backend.requestIrStates()
                    }
                    Label {
                        id: lblIR
                        text: "Esperando datos..."
                        color: "#fbc531"
                        font.pixelSize: 14
                        font.bold: true
                        Layout.leftMargin: 10
                    }
                }
            }
        }

        // ---------- 2. ACTUADORES: CINTA ----------
        GroupBox {
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                Label {
                    text: "CINTA TRANSPORTADORA"
                    color: "white"
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 20

                    Button {
                        text: "▶ ENCENDER"
                        onClicked: backend.encenderCinta()
                    }
                    Button {
                        text: "⏹ APAGAR"
                        onClicked: backend.apagarCinta()
                    }
                }
            }
        }

        // ---------- 3. ACTUADORES: SERVOS ----------
        GroupBox {
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                Label {
                    text: "SERVOMOTORES"
                    color: "white"
                    font.bold: true
                }

                // Fila Servo 1
                RowLayout {
                    Label { text: "Servo 1:"; color: "white"; Layout.preferredWidth: 60 }
                    Button { text: "0°"; onClicked: backend.setServo(0, 0) }
                    Button { text: "90°"; onClicked: backend.setServo(0, 90) }
                    Button { text: "180°"; onClicked: backend.setServo(0, 180) }
                }

                // Fila Servo 2
                RowLayout {
                    Label { text: "Servo 2:"; color: "white"; Layout.preferredWidth: 60 }
                    Button { text: "0°"; onClicked: backend.setServo(1, 0) }
                    Button { text: "90°"; onClicked: backend.setServo(1, 90) }
                    Button { text: "180°"; onClicked: backend.setServo(1, 180) }
                }

                // Fila Servo 3
                RowLayout {
                    Label { text: "Servo 3:"; color: "white"; Layout.preferredWidth: 60 }
                    Button { text: "0°"; onClicked: backend.setServo(2, 0) }
                    Button { text: "90°"; onClicked: backend.setServo(2, 90) }
                    Button { text: "180°"; onClicked: backend.setServo(2, 180) }
                }
            }
        }

        // Espaciador para empujar todo hacia arriba
        Item {
            Layout.fillHeight: true
        }
    }
}
