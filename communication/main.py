import sys
from PyQt6.QtWidgets import QApplication

from windows.main_menu import MainMenuWindow


def main():
    app = QApplication(sys.argv)
    window = MainMenuWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
