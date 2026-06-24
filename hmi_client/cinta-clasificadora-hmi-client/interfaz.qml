import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 450
    height: 700 // Aumentamos un poco el alto para hacerle lugar a la consola
    color: "#2b2b2b"

    // ==========================================
    // ESCUCHA DE EVENTOS DESDE C++
    // ==========================================
    Connections {
        target: backend

        function onDistanceUpdated(mm) {
            lblDistancia.text = mm + " mm"
        }

        function onIrStatesUpdated(ir0, ir1, ir2, ir3) {
            lblIR.text = "IR0: " + (ir0 ? "1" : "0") + " | " +
                         "IR1: " + (ir1 ? "1" : "0") + " | " +
                         "IR2: " + (ir2 ? "1" : "0") + " | " +
                         "IR3: " + (ir3 ? "1" : "0")
        }

        function onConnectionStatusChanged(connected) {
            if (!connected) {
                lblDistancia.text = "-- mm"
                lblIR.text = "Desconectado"
            }
        }

        // RECIBIMOS EL MENSAJE Y LO FORMATEAMOS
        function onLogMessageReceived(message) {
            // Obtenemos la hora actual del sistema
            var date = new Date();
            var hours = ("0" + date.getHours()).slice(-2);
            var minutes = ("0" + date.getMinutes()).slice(-2);
            var seconds = ("0" + date.getSeconds()).slice(-2);
            var timestamp = "[" + hours + ":" + minutes + ":" + seconds + "] ";

            // 1. Agregamos la línea a la consola
            consolaLog.append(timestamp + message);

            // 2. FORZAMOS EL AUTO-SCROLL hacia la última línea ingresada
            consolaLog.cursorPosition = consolaLog.length;
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

        // ---------- 1. TELEMETRÍA ----------
        GroupBox {
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                Label { text: "SENSORES (TIEMPO REAL)"; color: "white"; font.bold: true }
                RowLayout {
                    Label { text: "Distancia actual:"; color: "white" }
                    Label { id: lblDistancia; text: "-- mm"; color: "#66fcf1"; font.pixelSize: 16; font.bold: true; Layout.leftMargin: 10 }
                }
                RowLayout {
                    Label { text: "Sensores Ópticos:"; color: "white" }
                    Label { id: lblIR; text: "Esperando..."; color: "#fbc531"; font.pixelSize: 14; font.bold: true; Layout.leftMargin: 10 }
                }
            }
        }

        // ---------- 2. ACTUADORES: CINTA ----------
        GroupBox {
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                Label { text: "CINTA TRANSPORTADORA"; color: "white"; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 20
                    Button { text: "▶ ENCENDER"; onClicked: backend.encenderCinta() }
                    Button { text: "⏹ APAGAR"; onClicked: backend.apagarCinta() }
                }
            }
        }

        // ---------- 3. ACTUADORES: SERVOS ----------
        GroupBox {
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                Label { text: "SERVOMOTORES"; color: "white"; font.bold: true }
                RowLayout {
                    Label { text: "Servo 1:"; color: "white"; Layout.preferredWidth: 60 }
                    Button { text: "0°"; onClicked: backend.setServo(0, 0) }
                    Button { text: "90°"; onClicked: backend.setServo(0, 90) }
                    Button { text: "180°"; onClicked: backend.setServo(0, 180) }
                }
                RowLayout {
                    Label { text: "Servo 2:"; color: "white"; Layout.preferredWidth: 60 }
                    Button { text: "0°"; onClicked: backend.setServo(1, 0) }
                    Button { text: "90°"; onClicked: backend.setServo(1, 90) }
                    Button { text: "180°"; onClicked: backend.setServo(1, 180) }
                }
                RowLayout {
                    Label { text: "Servo 3:"; color: "white"; Layout.preferredWidth: 60 }
                    Button { text: "0°"; onClicked: backend.setServo(2, 0) }
                    Button { text: "90°"; onClicked: backend.setServo(2, 90) }
                    Button { text: "180°"; onClicked: backend.setServo(2, 180) }
                }
            }
        }

        // ---------- 4. CONSOLA DE REGISTRO (LOG) ----------
        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true // Le damos el espacio restante
            title: "Consola de Eventos" // El título nativo del GroupBox

            ScrollView {
                anchors.fill: parent
                clip: true // Evita que el texto salga del recuadro

                TextArea {
                    id: consolaLog
                    readOnly: true
                    color: "#00ff00" // Verde terminal hacker
                    background: Rectangle { color: "#1e1e1e" } // Fondo casi negro
                    font.family: "Courier" // Fuente monoespaciada tipo consola
                    font.pixelSize: 12
                    wrapMode: TextArea.Wrap // Si la línea es muy larga, baja abajo
                }
            }
        }
    }
}
