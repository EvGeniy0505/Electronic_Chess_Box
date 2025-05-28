from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QColor, QMouseEvent
from PyQt6.QtWidgets import QLabel, QGraphicsOpacityEffect


class ColorLabel(QLabel):
    clicked = pyqtSignal()

    def __init__(self, color: QColor, parent=None):
        super().__init__(parent)
        self._color = color
        self._enabled = True
        self.setFixedSize(60, 24)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self.update_style()

    def setEnabled(self, enabled: bool):
        super().setEnabled(enabled)
        self._enabled = enabled
        self.update_style()

    def setColor(self, color: QColor):
        self._color = color
        self.update_style()

    def update_style(self):
        border_color = "#666" if self._enabled else "#444"
        opacity = 1.0 if self._enabled else 0.4

        opacity_effect = QGraphicsOpacityEffect(self)
        opacity_effect.setOpacity(opacity)
        self.setGraphicsEffect(opacity_effect)

        self.setStyleSheet(
            f"""
            background-color: {self._color.name()};
            border: 1px solid {border_color};
            border-radius: 3px;
            """
        )

    def mousePressEvent(self, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton and self._enabled:
            self.clicked.emit()
