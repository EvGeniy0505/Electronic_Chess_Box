import os
import subprocess
import time

import berserk

FIFO_PATH_IN = "./build/lichess_in"
FIFO_PATH_OUT = "./build/lichess_out"

TOKEN = "lip_oJbvgEk60EQ65BEPWLlD"

session = berserk.TokenSession(TOKEN)
client = berserk.Client(session=session)


def get_current_game_id():
    games = client.games.get_ongoing()
    print(games)
    for game in games:
        if "gameId" in game:
            return game["gameId"]
    return None


def get_game_info(game_id):
    try:
        return client.games.export(game_id)
    except Exception as e:
        print(f"Ошибка при получении информации о игре: {e}")
        return None


def get_current_game_info():
    games = client.games.get_ongoing()
    print("Текущие игры:", games)
    if not games:
        return None, None, None, None

    game = games[0]
    game_id = game.get("gameId")
    fen = game.get("fen")
    my_color = game.get("color", "white").lower()
    is_my_turn = game.get("isMyTurn", False)

    print(f"Информация о игре {game_id}: цвет={my_color}, мой ход={is_my_turn}")
    return game_id, my_color, fen, is_my_turn


def listen_game_moves(game_id):
    stream = client.board.stream_game_state(game_id)
    for event in stream:
        print("Получено событие от Lichess:", event)
        if "moves" in event:
            moves = event["moves"].split()
            if moves:
                last_move = moves[-1]
                print(f"Последний ход: {last_move}")
                yield last_move
        elif "state" in event and event["state"] == "started":
            print("Игра началась")
            yield None


def send_move_to_engine(fifo, move):
    message = f"{move}\n" if move else "start\n"
    fifo.write(message)
    fifo.flush()
    print(f"[Lichess -> Engine] Отправлено: {message.strip()}")


def receive_move_from_engine(fifo):
    while True:
        line = fifo.readline()
        if not line:
            time.sleep(0.1)
            continue
        line = line.strip()
        if line.startswith("engine:"):
            move = line.split(":", 1)[1]
            print(f"[Engine -> Lichess] Получено: {move}")
            return move


def make_move_on_lichess(game_id, move):
    try:
        client.board.make_move(game_id, move)
        print(f"Ход {move} успешно отправлен на Lichess")
    except Exception as e:
        print(f"Ошибка при отправке хода {move} на Lichess: {e}")


if __name__ == "__main__":
    game_id, my_color, current_fen, is_my_turn = get_current_game_info()
    if not game_id:
        print("Нет активных игр для подключения")
        exit(1)

    print(f"Подключаемся к игре: {game_id}, играю за {my_color}, мой ход: {is_my_turn}")

    if not os.path.exists(FIFO_PATH_IN):
        os.mkfifo(FIFO_PATH_IN)

    if not os.path.exists(FIFO_PATH_OUT):
        os.mkfifo(FIFO_PATH_OUT)

    fd_in = os.open(FIFO_PATH_IN, os.O_RDWR)
    fifo_in = os.fdopen(fd_in, "w")
    send_move_to_engine(fifo_in, f"fen {current_fen}")
    engine_process = subprocess.Popen(
        [
            "./build/gui_chess",
            "--lichess",
            my_color,
        ],  # Замените на путь к вашему движку
    )

    with open(FIFO_PATH_OUT, "r") as fifo_out, open(FIFO_PATH_IN, "w") as fifo_in:
        # Отправляем текущую позицию
        print(f"Отправляю FEN движку: {current_fen}")

        # Если сейчас не наш ход, ждем ход противника
        if not is_my_turn:
            print("Ожидаю ход противника...")
            opponent_move = None
            while opponent_move is None:
                opponent_move = next(listen_game_moves(game_id))
                if opponent_move is None:
                    time.sleep(1)

            print(f"Противник сделал ход: {opponent_move}")
            send_move_to_engine(fifo_in, opponent_move)

        # Основной игровой цикл
        while True:
            # Получаем ход от движка
            print("Ожидаю ход от движка...")
            engine_move = receive_move_from_engine(fifo_out)
            print(f"Делаю ход: {engine_move}")
            make_move_on_lichess(game_id, engine_move)

            # Ждем ответа противника
            print("Ожидаю ход противника...")
            opponent_move = None
            while opponent_move is None:
                opponent_move = next(listen_game_moves(game_id))
                if opponent_move is None:
                    time.sleep(1)

            print(f"Противник сделал ход: {opponent_move}")
            send_move_to_engine(fifo_in, opponent_move)
