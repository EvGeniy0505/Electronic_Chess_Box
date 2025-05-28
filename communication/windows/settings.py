import json
import os

from constants import DEFAULT_SETTINGS, SETTINGS_FILE
from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QColor
from PyQt6.QtWidgets import (
    QCheckBox,
    QColorDialog,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QPushButton,
    QRadioButton,
    QSlider,
    QVBoxLayout,
    QWidget,
)
from themes.theme import apply_theme
from widgets.color_label import ColorLabel


class SettingsWindow(QMainWindow):
    theme_changed = pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self.settings = self._load_or_create_settings()
        self.color_selectors = []

        self.init_ui()
        apply_theme(self, self.settings["theme"])
        self.update_controls_state()

    def _on_theme_toggled(self):
        """Определяем какая тема выбрана"""
        theme = "dark" if self.dark_theme_radio.isChecked() else "light"
        self.settings["theme"] = theme
        apply_theme(self, theme)
        self.theme_changed.emit(theme)

    def _load_or_create_settings(self):
        """Загружает настройки из файла или создает новые"""
        if not os.path.exists(SETTINGS_FILE):
            return DEFAULT_SETTINGS.copy()

        try:
            with open(SETTINGS_FILE, "r") as f:
                data = json.load(f)

            settings = DEFAULT_SETTINGS.copy()

            # Преобразование цветов
            color_keys = [
                "light_color",
                "dark_color",
                "move_color",
                "opponent_move_color",
            ]
            for key in color_keys:
                if key in data and len(data[key]) == 4:
                    settings[key] = QColor(*data[key])

            # Другие настройки
            if "highlight_enabled" in data:
                settings["highlight_enabled"] = bool(data["highlight_enabled"])
            if "highlight_intensity" in data:
                settings["highlight_intensity"] = max(
                    10, min(100, int(data["highlight_intensity"]))
                )
            if "theme" in data and data["theme"] in ["dark", "light"]:
                settings["theme"] = data["theme"]

            return settings

        except Exception as e:
            print(f"Error loading settings: {e}")
            return DEFAULT_SETTINGS.copy()

    def init_ui(self):
        self.setWindowTitle("Настройки шахмат")
        self.setFixedSize(500, 450)

        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)

        # Группа выбора темы
        theme_group = QGroupBox("Тема приложения")
        theme_layout = QVBoxLayout()

        self.dark_theme_radio = QRadioButton("Тёмная тема")
        self.light_theme_radio = QRadioButton("Светлая тема")

        if self.settings["theme"] == "dark":
            self.dark_theme_radio.setChecked(True)
        else:
            self.light_theme_radio.setChecked(True)

        # Подключаем одну кнопку к слоту
        self.dark_theme_radio.toggled.connect(self._on_theme_toggled)

        theme_layout.addWidget(self.dark_theme_radio)
        theme_layout.addWidget(self.light_theme_radio)
        theme_group.setLayout(theme_layout)

        # Группа подсветки
        highlight_group = QGroupBox("Настройки подсветки")
        highlight_layout = QVBoxLayout()

        self.highlight_checkbox = QCheckBox("Подсвечивать клетки доски")
        self.highlight_checkbox.setChecked(self.settings["highlight_enabled"])
        self.highlight_checkbox.stateChanged.connect(self.on_highlight_changed)

        self.light_color_selector = self._create_color_selector(
            "Светлые клетки:", self.settings["light_color"], "light_color"
        )
        self.dark_color_selector = self._create_color_selector(
            "Тёмные клетки:", self.settings["dark_color"], "dark_color"
        )
        self.move_color_selector = self._create_color_selector(
            "Возможные ходы:", self.settings["move_color"], "move_color"
        )
        self.opponent_color_selector = self._create_color_selector(
            "Ход противника:",
            self.settings["opponent_move_color"],
            "opponent_move_color",
        )

        intensity_layout = QHBoxLayout()
        intensity_label = QLabel("Интенсивность подсветки:")
        self.intensity_slider = QSlider(Qt.Orientation.Horizontal)
        self.intensity_slider.setRange(10, 100)
        self.intensity_slider.setValue(self.settings["highlight_intensity"])
        self.intensity_slider.valueChanged.connect(self.update_intensity)

        intensity_layout.addWidget(intensity_label)
        intensity_layout.addWidget(self.intensity_slider)

        highlight_layout.addWidget(self.highlight_checkbox)
        highlight_layout.addWidget(self.light_color_selector["widget"])
        highlight_layout.addWidget(self.dark_color_selector["widget"])
        highlight_layout.addSpacing(15)
        highlight_layout.addWidget(self.move_color_selector["widget"])
        highlight_layout.addWidget(self.opponent_color_selector["widget"])
        highlight_layout.addLayout(intensity_layout)
        highlight_group.setLayout(highlight_layout)

        # Кнопки
        btn_layout = QHBoxLayout()
        self.apply_btn = QPushButton("Применить")
        self.cancel_btn = QPushButton("Отмена")

        self.apply_btn.clicked.connect(self._save_settings)
        self.cancel_btn.clicked.connect(self.close)

        btn_layout.addStretch()
        btn_layout.addWidget(self.apply_btn)
        btn_layout.addWidget(self.cancel_btn)

        # Основной layout
        layout.addWidget(theme_group)
        layout.addWidget(highlight_group)
        layout.addStretch()
        layout.addLayout(btn_layout)

    def _create_color_selector(self, text, color, setting_key):
        widget = QWidget()
        layout = QHBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)

        label = QLabel(text)
        color_label = ColorLabel(color)
        color_label.clicked.connect(
            lambda: self._change_color(setting_key, color_label)
        )

        layout.addWidget(label)
        layout.addStretch()
        layout.addWidget(color_label)

        return {
            "widget": widget,
            "color_label": color_label,
            "text_label": label,
            "setting_key": setting_key,
        }

    def _change_color(self, setting_key, color_label):
        current_color = self.settings[setting_key]
        new_color = QColorDialog.getColor(
            current_color, self, f"Выберите цвет для {setting_key}"
        )

        if new_color.isValid():
            color_label.setColor(new_color)
            self.settings[setting_key] = new_color
            if setting_key in ["move_color", "opponent_move_color"]:
                new_color.setAlpha(self.settings["highlight_intensity"])

    def update_intensity(self, value):
        self.settings["highlight_intensity"] = value
        for key in ["move_color", "opponent_move_color"]:
            self.settings[key].setAlpha(value)

    def on_highlight_changed(self, state):
        self.settings["highlight_enabled"] = state == Qt.CheckState.Checked.value
        self.update_controls_state()

    def update_controls_state(self):
        enabled = self.settings["highlight_enabled"]
        for selector in [self.light_color_selector, self.dark_color_selector]:
            selector["color_label"].setEnabled(enabled)
            selector["text_label"].setEnabled(enabled)

    def _save_settings(self):
        try:
            data = {
                "theme": self.settings["theme"],
                "highlight_enabled": self.settings["highlight_enabled"],
                "highlight_intensity": self.settings["highlight_intensity"],
            }

            color_keys = [
                "light_color",
                "dark_color",
                "move_color",
                "opponent_move_color",
            ]
            for key in color_keys:
                color = self.settings[key]
                data[key] = [color.red(), color.green(), color.blue(), color.alpha()]

            with open(SETTINGS_FILE, "w") as f:
                json.dump(data, f, indent=4)

            self.close()

        except Exception as e:
            print(f"Error saving settings: {e}")
