import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Sudoku

ApplicationWindow {
    id: win
    width: 520
    height: 800
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height
    visible: true
    title: qsTr("数独求解器")
    color: "#F2F4F7"
    font.family: "Microsoft YaHei UI"

    // ---------- 扁平按钮 ----------
    component FlatButton: Button {
        id: control
        property bool primary: false
        property bool toggle: false
        checkable: toggle
        hoverEnabled: true
        focusPolicy: Qt.NoFocus
        activeFocusOnTab: false
        implicitHeight: 40
        leftPadding: 12
        rightPadding: 12

        contentItem: Text {
            text: control.text
            font.pixelSize: 14
            font.weight: Font.Medium
            color: (control.primary || control.checked) ? "#FFFFFF"
                 : control.down ? "#0F6CBD"
                 : "#3A4250"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 6
            color: control.primary ? (control.down ? "#0B5AA7" : control.hovered ? "#0E68BA" : "#0F6CBD")
                 : control.checked ? "#0F6CBD"
                 : control.down ? "#E8EDF3"
                 : control.hovered ? "#F1F4F8"
                 : "#FFFFFF"
            border.width: (control.primary || control.checked) ? 0 : 1
            border.color: "#D3D9E0"
        }
    }

    // ---------- 键盘输入 ----------
    Item {
        id: keyArea
        anchors.fill: parent
        focus: true

        Keys.onPressed: (event) => {
            const k = event.key
            if (k >= Qt.Key_1 && k <= Qt.Key_9) {
                Engine.inputDigit(k - Qt.Key_0)
                event.accepted = true
            } else if (k >= Qt.Key_NumPad1 && k <= Qt.Key_NumPad9) {
                Engine.inputDigit(k - Qt.Key_NumPad1 + 1)
                event.accepted = true
            } else if (k === Qt.Key_0 || k === Qt.Key_NumPad0) {
                Engine.inputDigit(0)
                event.accepted = true
            } else if (k === Qt.Key_Backspace || k === Qt.Key_Delete) {
                Engine.inputDigit(0)
                event.accepted = true
            } else if (k === Qt.Key_Left)  { Engine.moveSel(0, -1); event.accepted = true }
            else if (k === Qt.Key_Right) { Engine.moveSel(0,  1); event.accepted = true }
            else if (k === Qt.Key_Up)    { Engine.moveSel(-1, 0); event.accepted = true }
            else if (k === Qt.Key_Down)  { Engine.moveSel( 1, 0); event.accepted = true }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            // ---------- 标题 ----------
            ColumnLayout {
                spacing: 2
                Text {
                    text: qsTr("数独")
                    font.pixelSize: 28
                    font.weight: Font.Bold
                    color: "#1F2328"
                }
                Text {
                    text: qsTr("扁平风格 · 键盘与鼠标均可操作")
                    font.pixelSize: 12
                    color: "#8A929B"
                }
            }

            // ---------- 棋盘卡片 ----------
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: board.boardSize + 20
                height: board.boardSize + 20
                radius: 12
                color: "#FFFFFF"
                border.width: 1
                border.color: "#E4E8EE"

                Item {
                    id: board
                    property int cellSize: 52
                    property int boardSize: cellSize * 9
                    anchors.centerIn: parent
                    width: boardSize
                    height: boardSize

                    // 格子
                    Repeater {
                        model: Engine.cells
                        delegate: Rectangle {
                            required property int index
                            required property var modelData
                            readonly property int r: Math.floor(index / 9)
                            readonly property int c: index % 9
                            readonly property bool isSel: index === Engine.selIndex
                            readonly property bool isPeer: Engine.selIndex >= 0 && !isSel &&
                                (r === Engine.selRow || c === Engine.selCol ||
                                 (Math.floor(r / 3) === Math.floor(Engine.selRow / 3) &&
                                  Math.floor(c / 3) === Math.floor(Engine.selCol / 3)))
                            readonly property bool sameVal: modelData > 0 && Engine.selIndex >= 0 &&
                                !isSel && modelData === Engine.cells[Engine.selIndex]

                            x: c * board.cellSize
                            y: r * board.cellSize
                            width: board.cellSize
                            height: board.cellSize
                            color: Engine.conflicts[index] ? "#FBEBEC"
                                 : isSel ? "#DCEBFC"
                                 : (isPeer || sameVal) ? "#EFF5FB"
                                 : hover.hovered ? "#F5F8FB"
                                 : "#FFFFFF"

                            Text {
                                anchors.centerIn: parent
                                text: modelData > 0 ? modelData : ""
                                font.pixelSize: 24
                                font.weight: Font.DemiBold
                                color: Engine.conflicts[index] ? "#D64545"
                                     : Engine.givens[index] ? "#20242A"
                                     : "#0F6CBD"
                            }

                            HoverHandler { id: hover }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Engine.select(r, c)
                            }
                        }
                    }

                    // 网格线：每 3 格加粗
                    Repeater {
                        model: 10
                        delegate: Rectangle {
                            required property int index
                            readonly property bool thick: index % 3 === 0
                            x: index === 0 ? 0 : (thick ? index * board.cellSize - 1 : index * board.cellSize - 0.5)
                            y: 0
                            width: thick ? 2 : 1
                            height: board.boardSize
                            color: thick ? "#9AA3AD" : "#D5DAE0"
                        }
                    }
                    Repeater {
                        model: 10
                        delegate: Rectangle {
                            required property int index
                            readonly property bool thick: index % 3 === 0
                            x: 0
                            y: index === 0 ? 0 : (thick ? index * board.cellSize - 1 : index * board.cellSize - 0.5)
                            width: board.boardSize
                            height: thick ? 2 : 1
                            color: thick ? "#9AA3AD" : "#D5DAE0"
                        }
                    }
                }
            }

            // ---------- 难度选择 ----------
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 8
                Text {
                    text: qsTr("难度")
                    font.pixelSize: 13
                    color: "#8A929B"
                    Layout.rightMargin: 4
                }
                Repeater {
                    model: 4
                    delegate: FlatButton {
                        required property int index
                        text: [qsTr("简单"), qsTr("中等"), qsTr("困难"), qsTr("专家")][index]
                        toggle: true
                        checked: Engine.difficulty === index
                        Layout.preferredWidth: 106
                        onClicked: Engine.setDifficulty(index)
                    }
                }
            }

            // ---------- 操作按钮 ----------
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 8
                Text {
                    text: qsTr("操作")
                    font.pixelSize: 13
                    color: "#8A929B"
                    Layout.rightMargin: 4
                }
                FlatButton {
                    text: qsTr("新游戏")
                    primary: true
                    Layout.preferredWidth: 102
                    onClicked: Engine.newGame()
                }
                FlatButton {
                    text: qsTr("示例题目")
                    Layout.preferredWidth: 102
                    onClicked: Engine.loadSample()
                }
                FlatButton {
                    text: qsTr("求解")
                    Layout.preferredWidth: 102
                    onClicked: Engine.solveCurrent()
                }
                FlatButton {
                    text: qsTr("清空")
                    Layout.preferredWidth: 102
                    onClicked: Engine.clearBoard()
                }
            }

            // ---------- 数字键盘 ----------
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 6
                Repeater {
                    model: 10
                    delegate: FlatButton {
                        required property int index
                        text: index === 9 ? "⌫" : String(index + 1)
                        implicitHeight: 42
                        Layout.preferredWidth: 42
                        onClicked: Engine.inputDigit(index === 9 ? 0 : index + 1)
                    }
                }
            }

            Item { Layout.fillHeight: true }

            // ---------- 状态栏 ----------
            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: Engine.statusText
                font.pixelSize: 12
                color: Engine.statusError ? "#D64545" : "#8A929B"
                wrapMode: Text.Wrap
            }
        }
    }
}
