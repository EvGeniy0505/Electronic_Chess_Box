import sys
import os
from PyQt6.QtWidgets import QApplication

def main():
    # Создаем QApplication первым делом
    app = QApplication(sys.argv)
    
    # Настройка путей
    os.makedirs("./build", exist_ok=True)
    
    try:
        from windows.main_menu import MainMenuWindow
        from arduino_bridge import start_arduino_bridge
        
        # Запускаем Arduino bridge
        arduino_process = start_arduino_bridge()
        
        # Создаем главное окно
        window = MainMenuWindow()
        window.show()
        
        # Запускаем основной цикл
        exit_code = app.exec()
        
        # Завершаем процесс Arduino bridge при закрытии
        if arduino_process:
            arduino_process.terminate()
        
        sys.exit(exit_code)
        
    except Exception as e:
        from PyQt6.QtWidgets import QMessageBox
        QMessageBox.critical(None, "Fatal Error", f"Application failed to start: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()