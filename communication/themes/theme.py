from constants import THEMES
from PyQt6.QtGui import QColor, QPalette
from PyQt6.QtWidgets import QWidget


def apply_theme(widget: QWidget, theme_name: str):
    """Применяет выбранную тему к виджету"""
    theme = THEMES.get(theme_name, THEMES["dark"])

    palette = widget.palette()
    palette.setColor(QPalette.ColorRole.Window, QColor(theme["background"]))
    palette.setColor(QPalette.ColorRole.WindowText, QColor(theme["text"]))
    palette.setColor(QPalette.ColorRole.Base, QColor(theme["background"]))
    palette.setColor(QPalette.ColorRole.Text, QColor(theme["text"]))
    palette.setColor(QPalette.ColorRole.Button, QColor(theme["button"]))
    palette.setColor(QPalette.ColorRole.ButtonText, QColor(theme["text"]))

    widget.setPalette(palette)

    widget.setStyleSheet(
        f"""
        QWidget {{
            background-color: {theme["background"]};
            color: {theme["text"]};
        }}
        QPushButton {{
            font-size: 16px;
            padding: 10px;
            color: {theme["text"]};
            background-color: {theme["button"]};
            border: 1px solid {theme["border"]};
            border-radius: 4px;
        }}
        QPushButton:hover {{
            background-color: {theme["button_hover"]};
        }}
        QLabel {{
            color: {theme["text"]};
        }}
    """
    )
