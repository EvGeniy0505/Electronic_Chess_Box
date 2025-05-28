#pragma once
#include "board/board.hpp"
#include "engine/computer_player.hpp"
#include "pieces/piece.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <string>

class SDLGame {
  public:
    SDLGame(bool vsComputer = false, bool vsLichess = false,
            chess::Color computerColor = chess::Color::BLACK);
    ~SDLGame();
    void run();

  private:
    int fifo_in_fd = -1;  // для чтения ходов от Python
    int fifo_out_fd = -1; // для записи ходов в Python
    const std::string fifo_in_path = "./build/lichess_in";
    const std::string fifo_out_path = "./build/lichess_out";
    void initFIFO();
    void sendMoveToPython(const std::string &move);

    void renderGame();
    void processFIFOMove();
    void initializeFromFIFO();
    bool playerMoveFromTerminal();
    void initSDL();
    void renderBoard();
    void renderPieces();
    void drawPiece(const chess::Piece &piece, const SDL_Rect &rect);
    void handleEvents();
    void handleMouseDown(const SDL_Event &event);
    void handleMouseMotion(const SDL_Event &event);
    void handleMouseUp(const SDL_Event &event);
    void makeComputerMove();
    void cleanup();
    void renderGameOverMessage();
    void renderNewGameButton();
    bool isNewGameButtonClicked(int x, int y);
    void renderPromotionDialog(int x, int y);
    chess::PieceType showPromotionDialog(chess::Color playerColor);

    bool isPromoting;
    int promotionX, promotionY;
    std::array<chess::PieceType, 4> promotionOptions;

    bool gameOver;
    std::string gameOverMessage;
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    SDL_Texture *piecesTexture;
    chess::Board board;
    std::unique_ptr<chess::engine::ComputerPlayer> computer;
    bool isRunning;
    bool isDragging;
    bool vsComputer;
    bool vsLichess;
    int dragStartX, dragStartY;
    SDL_Rect dragRect;
    chess::Piece draggedPiece;
    std::vector<std::pair<int, int>> possibleMoves;
};
