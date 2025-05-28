from PyQt6.QtGui import QColor

DEFAULT_SETTINGS = {
    "theme": "dark",
    "highlight_enabled": True,
    "light_color": QColor(240, 217, 181),
    "dark_color": QColor(181, 136, 99),
    "move_color": QColor(100, 200, 100, 150),
    "opponent_move_color": QColor(200, 100, 100, 150),
    "highlight_intensity": 80,
}

# Стили для разных тем
THEMES = {
    "dark": {
        "background": "#2d2d2d",
        "text": "#f0f0f0",
        "button": "#3a3a3a",
        "button_hover": "#4a4a4a",
        "border": "#555",
    },
    "light": {
        "background": "#f0f0f0",
        "text": "#333333",
        "button": "#e0e0e0",
        "button_hover": "#d0d0d0",
        "border": "#aaa",
    },
}
SETTINGS_FILE = "settings.json"

STYLESHEET = """
    QWidget {
        background-color: #2d2d2d;
        color: #f0f0f0;
    }
    QPushButton {
        font-size: 16px;
        padding: 10px;
        color: #f0f0f0;
        background-color: #3a3a3a;
        border: 1px solid #555;
        border-radius: 4px;
    }
    QPushButton:hover {
        background-color: #4a4a4a;
    }
    QLabel {
        color: #f0f0f0;
    }
"""
