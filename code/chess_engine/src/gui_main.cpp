#include "engine/computer_player.hpp"
#include "gui/sdl_game.hpp"
#include <iostream>

int main(int argc, char *argv[]) {

    bool vsComputer = false;
    bool vsLichess = false;
    chess::Color computerColor = chess::Color::BLACK;

    // Парсинг аргументов командной строки
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--computer" || arg == "-c") {
            vsComputer = true;
            if (i + 1 < argc) {
                std::string color = argv[++i];
                if (color == "white")
                    computerColor = chess::Color::WHITE;
            }
        }
        if (arg == "--lichess" || arg == "-c") {
            vsLichess = true;
            if (i + 1 < argc) {
                std::string color = argv[++i];
                if (color == "white")
                    computerColor = chess::Color::WHITE;
            }
        }
    }

    try {
        SDLGame game(vsComputer, vsLichess, true, computerColor);

        game.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
