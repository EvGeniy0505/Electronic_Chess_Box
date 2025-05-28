#include "sdl_game.hpp"
#include "board/initialization.hpp"
#include <chrono>
#include <cstring>
#include <errno.h>
#include <fcntl.h> // open
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <sys/stat.h> // mkfifo
#include <thread>
#include <unistd.h> // read, write, close

auto toChessNotation = [](int x, int y) {
    char file = 'a' + x;
    char rank = '1' + (7 - y); // отражаем по оси Y
    std::string s;
    s += file;
    s += rank;
    return s;
};

void SDLGame::sendMoveToPython(const std::string &move) {
    if (fifo_out_fd == -1)
        return;

    std::cout << move << std::endl;
    std::string message = "engine:" + move + "\n";
    write(fifo_out_fd, message.c_str(), message.size());
}

bool SDLGame::playerMoveFromTerminal() {
    std::string line;
    std::string from, to;

    std::cout << "Введите ход (например: e2 e4): ";
    if (!std::getline(std::cin, line)) {
        isRunning = false;
        return false;
    }

    std::istringstream iss(line);
    if (!(iss >> from >> to) || from.size() != 2 || to.size() != 2) {
        std::cout << "Неверный формат. Используйте, например: e2 e4\n";
        return false;
    }

    auto fileCharToX = [](char file) -> int {
        return file - 'a'; // 'a' -> 0, 'b' -> 1, ..., 'h' -> 7
    };

    auto rankCharToY = [](char rank) -> int {
        return 8 - (rank - '0'); // '2' -> 6, '4' -> 4
    };

    int fromX = fileCharToX(from[0]);
    int fromY = rankCharToY(from[1]);
    int toX = fileCharToX(to[0]);
    int toY = rankCharToY(to[1]);

    // Проверка диапазонов
    if (fromX < 0 || fromX > 7 || toX < 0 || toX > 7 || fromY < 0 ||
        fromY > 7 || toY < 0 || toY > 7) {
        std::cout << "Неверные координаты. Попробуйте снова.\n";
        return false;
    }

    // Проверяем, что на from есть фигура текущего игрока
    const auto &piece = board.get_piece({fromX, fromY});
    if (piece.get_type() == chess::PieceType::NONE ||
        piece.get_color() != board.current_player) {
        std::cout
            << "На начальной клетке нет вашей фигуры. Попробуйте ещё раз.\n";
        return false;
    }

    // Проверяем легальность хода
    auto legalMoves = board.get_legal_moves({fromX, fromY});
    bool moveIsLegal = false;
    for (const auto &m : legalMoves) {
        if (m.first == toX && m.second == toY) {
            moveIsLegal = true;
            break;
        }
    }

    if (!moveIsLegal) {
        std::cout << "Нелегальный ход. Попробуйте ещё раз.\n";
        return false;
    }

    // Превращение пешки
    bool isPromotionMove =
        (piece.get_type() == chess::PieceType::PAWN) &&
        ((piece.get_color() == chess::Color::WHITE && toY == 0) ||
         (piece.get_color() == chess::Color::BLACK && toY == 7));

    if (isPromotionMove) {
        board.make_move({fromX, fromY}, {toX, toY}, chess::PieceType::QUEEN);
        std::cout << "Пешка превращена в ферзя.\n";
    } else {
        board.make_move({fromX, fromY}, {toX, toY});
    }

    std::cout << "Ваш ход: " << from << " -> " << to << "  (" << fromX << fromY
              << " -> " << toX << toY << ")\n";

    renderGame();

    if (vsLichess) {
        std::string move =
            toChessNotation(fromX, fromY) + toChessNotation(toX, toY);
        sendMoveToPython(move);
    }

    return true;
}

SDLGame::SDLGame(bool vsComputer, bool vsLichess, chess::Color computerColor)
    : window(nullptr), renderer(nullptr), font(nullptr), piecesTexture(nullptr),
      isRunning(true), isDragging(false), vsComputer(vsComputer),
      vsLichess(vsLichess), dragStartX(-1), dragStartY(-1),
      computer(chess::engine::ComputerPlayer::create(computerColor, 3)),
      board(), gameOver(false) {
    initSDL();
    std::cerr << "Error: " << std::endl;

    if (vsLichess)
        initFIFO();
}

void SDLGame::initFIFO() {
    // Создаем FIFO, если еще не существуют
    // if (mkfifo(fifo_in_path.c_str(), 0666) == -1 && errno != EEXIST) {
    //     std::cerr << "Error creating input FIFO: " << strerror(errno)
    //               << std::endl;
    // }
    // if (mkfifo(fifo_out_path.c_str(), 0666) == -1 && errno != EEXIST) {
    //     std::cerr << "Error creating output FIFO: " << strerror(errno)
    //               << std::endl;
    // }

    // Открываем FIFO для чтения в неблокирующем режиме
    fifo_in_fd = open(fifo_in_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fifo_in_fd == -1) {
        std::cerr << "Error opening input FIFO: " << strerror(errno)
                  << std::endl;
    }

    // Открываем FIFO для записи в неблокирующем режиме
    fifo_out_fd = open(fifo_out_path.c_str(), O_WRONLY | O_NONBLOCK);
    if (fifo_out_fd == -1) {
        std::cerr << "Error opening output FIFO: " << strerror(errno)
                  << std::endl;
    }

    // Проверяем, что оба канала открыты успешно
    if (fifo_in_fd != -1 && fifo_out_fd != -1) {
        std::cout << "FIFO channels initialized successfully" << std::endl;
    } else {
        std::cerr << "Failed to initialize FIFO channels" << std::endl;
    }
}

SDLGame::~SDLGame() { cleanup(); }

void SDLGame::initSDL() {
    // Инициализация SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error("SDL_Init Error: " +
                                 std::string(SDL_GetError()));
    }

    // Создание окна
    window =
        SDL_CreateWindow("Chess Game", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 800, 800, SDL_WINDOW_SHOWN);
    if (!window) {
        throw std::runtime_error("SDL_CreateWindow Error: " +
                                 std::string(SDL_GetError()));
    }

    // Создание рендерера
    renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_DestroyWindow(window);
        throw std::runtime_error("SDL_CreateRenderer Error: " +
                                 std::string(SDL_GetError()));
    }

    // Инициализация SDL_image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        throw std::runtime_error("SDL_image could not initialize: " +
                                 std::string(IMG_GetError()));
    }

    // Загрузка текстуры с фигурами
    piecesTexture =
        IMG_LoadTexture(renderer, "./assets/images/chess_pieces.png");
    if (!piecesTexture) {
        std::cerr << "Warning: Failed to load chess pieces texture: "
                  << IMG_GetError() << std::endl;
    }

    // Инициализация SDL_ttf (для текстового фолбэка)
    if (TTF_Init() == -1) {
        std::cerr << "Warning: SDL_ttf could not initialize: " << TTF_GetError()
                  << std::endl;
    }
}

void SDLGame::renderBoard() {
    // Получаем позицию короля текущего игрока, если ему шах
    std::optional<std::pair<int, int>> kingInCheckPos;
    if (board.is_check(board.current_player)) {
        // Находим позицию короля
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                const auto &piece = board.get_piece({x, y});
                if (piece.get_type() == chess::PieceType::KING &&
                    piece.get_color() == board.current_player) {
                    kingInCheckPos = {x, y};
                    break;
                }
            }
            if (kingInCheckPos)
                break;
        }
    }

    // Рисуем шахматную доску
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            SDL_Rect rect = {x * 100, y * 100, 100, 100};

            // Проверяем, нужно ли подсветить клетку красным (король под шахом)
            if (kingInCheckPos && kingInCheckPos->first == x &&
                kingInCheckPos->second == y) {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0,
                                       255); // Красный цвет для шаха
            }
            // Подсветка возможных ходов
            else {
                bool isHighlight = false;
                for (const auto &move : possibleMoves) {
                    if (move.first == x && move.second == y) {
                        isHighlight = true;
                        break;
                    }
                }

                if (isHighlight) {
                    SDL_SetRenderDrawColor(renderer, 100, 200, 100,
                                           255); // Зеленый для возможных ходов
                } else if ((x + y) % 2 == 0) {
                    SDL_SetRenderDrawColor(renderer, 240, 217, 181,
                                           255); // Светлая клетка
                } else {
                    SDL_SetRenderDrawColor(renderer, 181, 136, 99,
                                           255); // Темная клетка
                }
            }

            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

void SDLGame::drawPiece(const chess::Piece &piece, const SDL_Rect &rect) {
    if (piece.get_type() == chess::PieceType::NONE)
        return;

    // 1. Попытка использовать графические спрайты
    if (piecesTexture) {
        // Размер одной фигуры в спрайте (64x64)
        const int pieceSize = 60;
        SDL_Rect srcRect = {0, 0, pieceSize, pieceSize};

        // Выбираем колонку в зависимости от типа фигуры
        switch (piece.get_type()) {
            case chess::PieceType::QUEEN:
                srcRect.x = 0;
                break;
            case chess::PieceType::KING:
                srcRect.x = pieceSize;
                break;
            case chess::PieceType::ROOK:
                srcRect.x = pieceSize * 2;
                break;
            case chess::PieceType::KNIGHT:
                srcRect.x = pieceSize * 3;
                break;
            case chess::PieceType::BISHOP:
                srcRect.x = pieceSize * 4;
                break;
            case chess::PieceType::PAWN:
                srcRect.x = pieceSize * 5;
                break;
            default:
                return;
        }

        // Выбираем строку в зависимости от цвета
        srcRect.y = (piece.get_color() == chess::Color::WHITE) ? pieceSize : 0;

        const int drawSize = 100; // Новый размер фигуры

        // Центрирование фигуры в клетке
        SDL_Rect destRect = {
            rect.x + (rect.w - drawSize) / 2, // Центр по X
            rect.y + (rect.h - drawSize) / 2, // Центр по Y
            drawSize,                         // Новая ширина
            drawSize                          // Новая высота
        };

        // Рисуем фигуру
        SDL_RenderCopy(renderer, piecesTexture, &srcRect, &destRect);
        return;
    }

    // 2. Fallback: рисуем простые цветные круги
    SDL_Color color = piece.get_color() == chess::Color::WHITE
                          ? SDL_Color{255, 255, 255, 255}
                          : SDL_Color{50, 50, 50, 255};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void SDLGame::renderPieces() {
    // Сначала рисуем все фигуры, кроме перетаскиваемой
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            if (isDragging && x == dragStartX && y == dragStartY)
                continue;

            const auto &piece = board.get_piece({x, y});
            if (piece.get_type() == chess::PieceType::NONE)
                continue;

            SDL_Rect rect = {x * 100 + 18, y * 100 + 18, 64, 64};
            drawPiece(piece, rect);
        }
    }

    // Затем рисуем перетаскиваемую фигуру поверх остальных
    if (isDragging) {
        drawPiece(draggedPiece, dragRect);
    }
}

void SDLGame::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym ==
                    SDLK_t) { // Клавиша 't' для ввода из терминала
                    if (!vsComputer ||
                        board.current_player != computer->color_) {
                        playerMoveFromTerminal();
                    }
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                handleMouseDown(event);
                break;
            case SDL_MOUSEMOTION:
                handleMouseMotion(event);
                break;
            case SDL_MOUSEBUTTONUP:
                handleMouseUp(event);
                break;
        }
    }
}

void SDLGame::handleMouseDown(const SDL_Event &event) {
    if (gameOver) {
        if (isNewGameButtonClicked(event.button.x, event.button.y)) {
            gameOver = false;
            board = chess::Board();
            if (vsComputer && computer->color_ == chess::Color::WHITE) {
                makeComputerMove();
            }
        }
        return;
    }

    // Добавляем проверку для Lichess - можно двигать только свои фигуры
    bool canMove = (!vsComputer || board.current_player != computer->color_) &&
                   (!vsLichess || board.current_player == computer->color_);

    if (canMove && event.button.button == SDL_BUTTON_LEFT) {
        int mouseX = event.button.x;
        int mouseY = event.button.y;
        dragStartX = mouseX / 100;
        dragStartY = mouseY / 100;

        const auto &piece = board.get_piece({dragStartX, dragStartY});
        if (piece.get_type() != chess::PieceType::NONE &&
            piece.get_color() == board.current_player) {

            isDragging = true;
            draggedPiece = piece;
            dragRect = {mouseX - 25, mouseY - 25, 50, 50};
            possibleMoves = board.get_legal_moves({dragStartX, dragStartY});
        }
    }
}

void SDLGame::handleMouseMotion(const SDL_Event &event) {
    if (isDragging) {
        dragRect.x = event.motion.x - 25;
        dragRect.y = event.motion.y - 25;
    }
}

void SDLGame::handleMouseUp(const SDL_Event &event) {
    if (isDragging && event.button.button == SDL_BUTTON_LEFT) {
        isDragging = false;

        int mouseX = event.button.x;
        int mouseY = event.button.y;
        int targetX = mouseX / 100;
        int targetY = mouseY / 100;

        const auto &piece = board.get_piece({dragStartX, dragStartY});

        // Проверяем легальность хода ДО его выполнения
        auto legalMoves = board.get_legal_moves({dragStartX, dragStartY});
        bool moveIsLegal = false;
        for (const auto &m : legalMoves) {
            if (m.first == targetX && m.second == targetY) {
                moveIsLegal = true;
                break;
            }
        }

        if (!moveIsLegal) {
            possibleMoves.clear();
            return; // Нелегальный ход - выходим
        }

        // Проверяем, достигла ли пешка последней горизонтали
        bool isPromotionMove =
            (piece.get_type() == chess::PieceType::PAWN) &&
            ((piece.get_color() == chess::Color::WHITE && targetY == 0) ||
             (piece.get_color() == chess::Color::BLACK && targetY == 7));

        if (isPromotionMove) {
            promotionX = targetX;
            promotionY = targetY;
            chess::PieceType promotionChoice =
                showPromotionDialog(piece.get_color());

            if (promotionChoice != chess::PieceType::NONE) {
                board.make_move({dragStartX, dragStartY}, {targetX, targetY},
                                promotionChoice);
            }
        } else {
            board.make_move({dragStartX, dragStartY}, {targetX, targetY});
        }

        renderGame();
        if (vsLichess) {
            std::string move = toChessNotation(dragStartX, dragStartY) +
                               toChessNotation(targetX, targetY);
            sendMoveToPython(move);
        }

        possibleMoves.clear();

        // Ход компьютера, если играем против ИИ
        // if (vsComputer && board.current_player == computer->color_) {
        //     makeComputerMove();
        // }
    }
}

void SDLGame::makeComputerMove() {
    if (computer->makeMove(board)) {
        auto lastMove = computer->getLastMove(); // Нужно реализовать, если нет
        std::cout << "Компьютер ходит: " 
                  << toChessNotation(lastMove.from.first, lastMove.from.second)
                  << " -> "
                  << toChessNotation(lastMove.to.first, lastMove.to.second)
                  << std::endl;

        if (board.is_checkmate(computer->color_ == chess::Color::WHITE
                                   ? chess::Color::BLACK
                                   : chess::Color::WHITE)) {
            std::cout << "Checkmate! Computer wins!" << std::endl;
        }
    }
}

void SDLGame::initializeFromFIFO() {
    if (fifo_in_fd == -1) {
        std::cerr << "FIFO не инициализирован" << std::endl;
        chess::BoardInitializer::setup_initial_position(
            board, chess::BoardInitializer::STANDARD_FEN);
        return;
    }

    std::string initial_data;
    char buffer[256];
    ssize_t bytes_read;

    // Блокирующее чтение первого байта (ждем любые данные)
    bytes_read = read(fifo_in_fd, buffer, 1);
    if (bytes_read <= 0) {
        std::cerr << "Ошибка чтения из FIFO: " << strerror(errno) << std::endl;
        chess::BoardInitializer::setup_initial_position(
            board, chess::BoardInitializer::STANDARD_FEN);
        return;
    }

    // Первый байт получен, добавляем его в строку
    initial_data.push_back(buffer[0]);

    // Устанавливаем неблокирующий режим для оставшихся данных
    int flags = fcntl(fifo_in_fd, F_GETFL, 0);
    fcntl(fifo_in_fd, F_SETFL, flags | O_NONBLOCK);

    // Читаем остальные данные (если есть)
    while ((bytes_read = read(fifo_in_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        initial_data.append(buffer);

        if (initial_data.find('\n') != std::string::npos)
            break;
    }

    // Восстанавливаем блокирующий режим
    fcntl(fifo_in_fd, F_SETFL, flags);

    // Удаляем завершающий перевод строки если есть
    if (!initial_data.empty() && initial_data.back() == '\n')
        initial_data.pop_back();

    std::cout << "Получены данные: " << initial_data << std::endl;

    if (initial_data.find("fen ") == 0) {
        std::string fen = initial_data.substr(4);
        chess::BoardInitializer::setup_initial_position(board, fen);
        std::cout << "Инициализировано из FEN: " << fen << std::endl;
    } else {
        std::cerr << "Неизвестный формат инициализации: " << initial_data << std::endl;
                  
        chess::BoardInitializer::setup_initial_position(
            board, chess::BoardInitializer::STANDARD_FEN);
    }
}

void SDLGame::processFIFOMove() {
    if (fifo_in_fd == -1)
        return;

    char buffer[10] = {0};
    ssize_t bytes_read = read(fifo_in_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        std::string input(buffer, bytes_read);
        // Ожидаем формат "lichess:e2e4\n"
        // if (input.find("lichess:") == 0) {
        std::string move = input; // input.substr(8);
        std::cout << "Move from FIFO: " << move << std::endl;
        move = move.substr(0, move.find('\n')); // Удаляем символ новой строки

        // Преобразуем формат "e2e4" в "e2 e4"
        if (move.size() >= 4) {
            std::string from = move.substr(0, 2);
            std::string to = move.substr(2, 2);

            // std::cout << "Move from FIFO1: " << std::endl;
            // Дальше ваша существующая логика обработки хода
            auto fileCharToX = [](char file) -> int { return file - 'a'; };
            auto rankCharToY = [](char rank) -> int {
                return 8 - (rank - '0');
            };
            // std::cout << "Move from FIFO2: " << std::endl;

            int fromX = fileCharToX(from[0]);
            int fromY = rankCharToY(from[1]);
            int toX = fileCharToX(to[0]);
            int toY = rankCharToY(to[1]);

            if (fromX < 0 || fromX > 7 || toX < 0 || toX > 7 || fromY < 0 ||
                fromY > 7 || toY < 0 || toY > 7)
                return;

            // std::cout << "Move from FIFO3: " << std::endl;
            const auto &piece = board.get_piece({fromX, fromY});
            if (piece.get_type() == chess::PieceType::NONE ||
                piece.get_color() != board.current_player)
                return;

            // std::cout << "Move from FIFO3: " << std::endl;
            auto legalMoves = board.get_legal_moves({fromX, fromY});
            bool moveIsLegal = false;
            for (const auto &m : legalMoves) {
                if (m.first == toX && m.second == toY) {
                    moveIsLegal = true;
                    break;
                }
            }
            // std::cout << "Move from FIFO4: " << std::endl;
            if (!moveIsLegal)
                return;

            // std::cout << "Move from FIFO5: " << std::endl;
            bool isPromotionMove =
                (piece.get_type() == chess::PieceType::PAWN) &&
                ((piece.get_color() == chess::Color::WHITE && toY == 0) ||
                 (piece.get_color() == chess::Color::BLACK && toY == 7));

            // std::cout << "Move from FIFO6: " << std::endl;
            if (isPromotionMove) {
                board.make_move({fromX, fromY}, {toX, toY},
                                chess::PieceType::QUEEN);
            } else {
                board.make_move({fromX, fromY}, {toX, toY});
            }

            // std::cout << "Move from FIFO: " << from << " -> " << to
            // << std::endl;
        }
        // }
    }
}

void SDLGame::run() {

    if (vsLichess) {
        initializeFromFIFO();
    }
    renderGame();

    while (isRunning) {
        handleEvents();
        renderGame();

        // std::cout << (board.current_player == chess::Color::WHITE) <<
        // std::endl;
        if (!gameOver) {
            if (vsLichess && board.current_player != computer->color_) {
                processFIFOMove();

            } else if (vsComputer && board.current_player == computer->color_ &&
                       !isDragging) {
                makeComputerMove();
            }

            // Проверка окончания игры
            if (board.is_checkmate(chess::Color::WHITE)) {
                gameOver = true;
                gameOverMessage = "Чёрные победили! Это мааааат!";
            } else if (board.is_checkmate(chess::Color::BLACK)) {
                gameOver = true;
                gameOverMessage = "Белые победили! Это мааааат!";
            } else if (board.is_stalemate(board.current_player)) {
                gameOver = true;
                gameOverMessage = "Пат! Ничья!";
            }
        }
        renderGame();
    }
}

void SDLGame::renderGame() {

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    renderBoard();
    renderPieces();
    renderGameOverMessage();

    SDL_RenderPresent(renderer);
    // SDL_Delay(16);
}

void SDLGame::cleanup() {
    if (piecesTexture)
        SDL_DestroyTexture(piecesTexture);
    if (font)
        TTF_CloseFont(font);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);

    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    if (fifo_in_fd != -1) {
        close(fifo_in_fd);
        fifo_in_fd = -1;
    }
    if (fifo_out_fd != -1) {
        close(fifo_out_fd);
        fifo_out_fd = -1;
    }
}

// В sdl_game.cpp в методе renderGameOverMessage()

void SDLGame::renderGameOverMessage() {
    if (!gameOver)
        return;

    // Создаем полупрозрачный прямоугольник
    SDL_Rect overlay = {100, 340, 600, 200};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, &overlay);

    // Инициализируем шрифт (добавляем проверку и fallback)
    if (!font) {
        // Пробуем несколько путей к шрифту
        const char *fontPaths[] = {
            "arial.ttf",                          // Текущая директория
            "./assets/fonts/arial.ttf",           // Относительный путь
            "/usr/share/fonts/truetype/arial.ttf" // Абсолютный путь в Linux
        };

        for (const auto &path : fontPaths) {
            font = TTF_OpenFont(path, 36);
            if (font)
                break;
        }

        if (!font) {
            std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
            // Fallback - используем простой прямоугольник с рамкой
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &overlay);
            return;
        }
    }

    // Создаем текст в UTF-8 (важно для русского языка)
    const char *message = nullptr;
    if (gameOverMessage == "Чёрные победили! Это мааааат!") {
        message = u8"Чёрные победили! Это мааааат!";
    } else if (gameOverMessage == "Белые победили! Это мааааат!") {
        message = u8"Белые победили! Это мааааат!";
    } else {
        message = u8"Это паааат! Ничья!";
    }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, message, white);
    if (!surface) {
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    // Рендерим текст по центру
    SDL_Rect textRect = {400 - surface->w / 2, 400 - surface->h / 2, surface->w,
                         surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &textRect);

    // Освобождаем ресурсы
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);

    renderNewGameButton();
}

void SDLGame::renderNewGameButton() {
    SDL_Color buttonColor = {70, 70, 200, 255};
    SDL_Rect buttonRect = {300, 450, 200, 50};

    SDL_SetRenderDrawColor(renderer, buttonColor.r, buttonColor.g,
                           buttonColor.b, 255);
    SDL_RenderFillRect(renderer, &buttonRect);

    // Белая рамка
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &buttonRect);

    // Текст кнопки
    if (font) {
        SDL_Surface *textSurface =
            TTF_RenderUTF8_Blended(font, u8"Новая игра", {255, 255, 255, 255});
        if (textSurface) {
            SDL_Texture *textTexture =
                SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture) {
                SDL_Rect textRect = {400 - textSurface->w / 2,
                                     475 - textSurface->h / 2, textSurface->w,
                                     textSurface->h};
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }
    }
}

bool SDLGame::isNewGameButtonClicked(int x, int y) {
    SDL_Rect buttonRect = {300, 450, 200, 50};
    return (x >= buttonRect.x && x <= buttonRect.x + buttonRect.w &&
            y >= buttonRect.y && y <= buttonRect.y + buttonRect.h);
}

void SDLGame::renderPromotionDialog(int x, int y) {
    // Полупрозрачный фон
    SDL_Rect overlay = {0, 0, 800, 800};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &overlay);

    // Контейнер для вариантов превращения
    SDL_Rect dialogRect = {200, 300, 400, 200};
    SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
    SDL_RenderFillRect(renderer, &dialogRect);

    // Текст заголовка
    if (font) {
        SDL_Surface *textSurface = TTF_RenderUTF8_Blended(
            font, u8"Выберите фигуру:", {255, 255, 255, 255});
        if (textSurface) {
            SDL_Texture *textTexture =
                SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textRect = {400 - textSurface->w / 2, 320, textSurface->w,
                                 textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
            SDL_FreeSurface(textSurface);
        }
    }

    // Варианты фигур
    const std::array<std::pair<chess::PieceType, const char *>, 4> pieces = {
        std::make_pair(chess::PieceType::QUEEN, u8"Ферзь"),
        std::make_pair(chess::PieceType::ROOK, u8"Ладья"),
        std::make_pair(chess::PieceType::KNIGHT, u8"Конь"),
        std::make_pair(chess::PieceType::BISHOP, u8"Слон")};

    for (size_t i = 0; i < pieces.size(); ++i) {
        SDL_Rect pieceRect = {210 + (int)i * 100, 370, 80, 80};

        // Подсветка при наведении
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        if (mouseX >= pieceRect.x && mouseX <= pieceRect.x + pieceRect.w &&
            mouseY >= pieceRect.y && mouseY <= pieceRect.y + pieceRect.h) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        }
        SDL_RenderFillRect(renderer, &pieceRect);

        // Иконка фигуры
        chess::Piece tempPiece(pieces[i].first, board.current_player);
        drawPiece(tempPiece, pieceRect);

        // Название фигуры
        if (font) {
            SDL_Surface *textSurface = TTF_RenderUTF8_Blended(
                font, pieces[i].second, {255, 255, 255, 255});
            if (textSurface) {
                SDL_Texture *textTexture =
                    SDL_CreateTextureFromSurface(renderer, textSurface);
                SDL_Rect textRect = {200 + (int)i * 100 - textSurface->w / 2,
                                     460, textSurface->w, textSurface->h};
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
                SDL_FreeSurface(textSurface);
            }
        }
    }
}

chess::PieceType SDLGame::showPromotionDialog(chess::Color playerColor) {
    isPromoting = true;
    promotionOptions = {chess::PieceType::QUEEN, chess::PieceType::ROOK,
                        chess::PieceType::KNIGHT, chess::PieceType::BISHOP};
    chess::PieceType selected = chess::PieceType::NONE;

    while (isPromoting && isRunning) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isRunning = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = event.button.x;
                int mouseY = event.button.y;

                // Проверяем клик по вариантам
                for (size_t i = 0; i < promotionOptions.size(); ++i) {
                    SDL_Rect pieceRect = {250 + (int)i * 100, 370, 80, 80};
                    if (mouseX >= pieceRect.x &&
                        mouseX <= pieceRect.x + pieceRect.w &&
                        mouseY >= pieceRect.y &&
                        mouseY <= pieceRect.y + pieceRect.h) {
                        selected = promotionOptions[i];
                        isPromoting = false;
                        break;
                    }
                }
            }
        }

        renderGame();
    }

    return selected;
}
