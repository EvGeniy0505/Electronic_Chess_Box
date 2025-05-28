import json
import os
import signal
from typing import Dict, Optional

from constants import DEFAULT_SETTINGS, SETTINGS_FILE
from PyQt6.QtCore import QObject, QProcess, pyqtSignal
from PyQt6.QtWidgets import QPushButton, QVBoxLayout, QWidget
from themes.theme import apply_theme

from windows.settings import SettingsWindow


class GameProcess(QObject):
    finished = pyqtSignal(str)  # Сигнал с ключом процесса

    def __init__(self, key: str):
        super().__init__()
        self.key = key
        self.process = QProcess()
        self.process.finished.connect(self._on_finished)
        self.process.errorOccurred.connect(self._on_error)

    def start(self, command: list):
        self.process.start(command[0], command[1:])

    def terminate(self):
        if self.process.state() == QProcess.ProcessState.Running:
            self.process.terminate()

    def _on_finished(self):
        self.finished.emit(self.key)
        self.deleteLater()

    def _on_error(self, error):
        print(f"Process error: {error}")
        self._on_finished()


class MainMenuWindow(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.processes: Dict[str, Optional[GameProcess]] = {
            "local_game": None,
            "vs_computer": None,
            "lichess": None,
        }

        self.theme = self._load_theme()
        apply_theme(self, self.theme)
        self.init_ui()

    def _load_theme(self) -> str:
        """Загружает только настройку темы"""
        try:
            with open(SETTINGS_FILE, "r") as f:
                data = json.load(f)
                return data.get("theme", "dark")  # По умолчанию темная тема
        except:
            return "dark"

    def _update_theme(self, new_theme: str):
        """Обновляет тему после изменения настроек"""
        print(f"Received theme: {new_theme}")  # Для отладки
        if new_theme in ["dark", "light"] and new_theme != self.theme:
            self.theme = new_theme
            apply_theme(self, self.theme)
            # Обновляем все дочерние виджеты
            for child in self.findChildren(QWidget):
                apply_theme(child, self.theme)

    def init_ui(self):
        self.setWindowTitle("Главное меню")
        self.setFixedSize(400, 300)

        layout = QVBoxLayout(self)

        self.local_game_btn = QPushButton("Игра друг с другом")
        self.vs_computer_btn = QPushButton("Игра с компьютером")
        self.lichess_btn = QPushButton("Игра на Lichess")
        self.settings_btn = QPushButton("Настройки")

        layout.addStretch()
        layout.addWidget(self.local_game_btn)
        layout.addWidget(self.vs_computer_btn)
        layout.addWidget(self.lichess_btn)
        layout.addSpacing(20)
        layout.addWidget(self.settings_btn)
        layout.addStretch()

        self.local_game_btn.clicked.connect(self.start_local_game)
        self.vs_computer_btn.clicked.connect(self.start_vs_computer)
        self.lichess_btn.clicked.connect(self.start_lichess)
        self.settings_btn.clicked.connect(self.open_settings)

        self.update_buttons_state()

    def update_buttons_state(self):
        """Блокирует все кнопки, если выполняется хотя бы один процесс"""
        any_process_running = any(p is not None for p in self.processes.values())

        # Блокируем игровые кнопки, если есть запущенный процесс
        self.local_game_btn.setEnabled(not any_process_running)
        self.vs_computer_btn.setEnabled(not any_process_running)
        self.lichess_btn.setEnabled(not any_process_running)

        # Кнопка настроек всегда доступна
        self.settings_btn.setEnabled(True)

    def start_process(self, command: list, process_key: str):
        """Запускает процесс и блокирует все кнопки"""
        if any(p is not None for p in self.processes.values()):
            return

        process = GameProcess(process_key)
        process.finished.connect(self.on_process_finished)
        process.start(command)
        self.processes[process_key] = process
        self.update_buttons_state()

    def on_process_finished(self, process_key: str):
        """Обработчик завершения процесса"""
        if process_key in self.processes:
            self.processes[process_key] = None
        self.update_buttons_state()

    def start_local_game(self):
        self.start_process(["./build/gui_chess"], "local_game")

    def start_vs_computer(self):
        self.start_process(["./build/gui_chess", "--computer"], "vs_computer")

    def start_lichess(self):
        self.start_process(["python3", "lichess.py"], "lichess")

    def open_settings(self):
        self.settings_window = SettingsWindow()
        # Подключаем сигнал theme_changed вместо apply_btn.clicked
        self.settings_window.theme_changed.connect(self._update_theme)
        self.settings_window.show()

    def closeEvent(self, event):
        """При закрытии окна завершаем все процессы"""
        for key, process in self.processes.items():
            if process is not None:
                process.terminate()
        event.accept()
