import ctypes as c_
import pathlib
from enum import Enum
from collections import Counter

libjbname = pathlib.Path().absolute() / "libjamesbond.so"
c_lib = c_.CDLL(libjbname)


class PlayerType(Enum):
    RANDOM = 0
    QLEARNER = 1
    SHAPLEY = 2

class Action(Enum):
    RELOAD = 0
    SHIELD = 1
    SHOOT = 2

class PlayerState:
    def __init__(self, lives, bullets, remainingShields):
        c_lib.jb_createState.restype = c_.c_void_p
        self.c_state = c_lib.jb_createState(c_.c_int(lives), c_.c_int(bullets), c_.c_int(remainingShields))

    def __del__(self):
        c_lib.jb_destroyState.restype = None
        c_lib.jb_destroyState(c_.c_void_p(self.c_state))

    def lives(self):
        c_lib.jb_lives.restype = c_.c_int
        return c_lib.jb_lives(c_.c_void_p(self.c_state))

    def bullets(self):
        c_lib.jb_bullets.restype = c_.c_int
        return c_lib.jb_bullets(c_.c_void_p(self.c_state))

    def remainingShields(self):
        c_lib.jb_remainingShields.restype = c_.c_int
        return c_lib.jb_remainingShields(c_.c_void_p(self.c_state))

class Player:
    def __init__(self, type, seed):
        c_lib.jb_createPlayer.restype = c_.c_void_p
        self.c_player = c_lib.jb_createPlayer(c_.c_int(type.value), c_.c_int(seed))

    def __del__(self):
        c_lib.jb_destroyPlayer.restype = None
        c_lib.jb_destroyPlayer(c_.c_void_p(self.c_player))

    def play(self, ownState, opponentState):
        c_lib.jb_play.restype = c_.c_int
        action = c_lib.jb_play(c_.c_void_p(self.c_player), c_.c_void_p(ownState.c_state), c_.c_void_p(opponentState.c_state))
        return Action(action)


def applyActions(p0, p1, s0, s1, a0, a1):
    c_lib.jb_applyActions.restype = c_.c_int
    rc = c_lib.jb_applyActions(c_.c_void_p(p0.c_player), c_.c_void_p(p1.c_player), \
                               c_.c_void_p(s0.c_state), c_.c_void_p(s1.c_state), \
                               c_.c_int(a0.value), c_.c_int(a1.value))
    if rc < 0:
        raise Exception("error while applying action")

def playGame(p0, p1):
    s0 = PlayerState(5, 0, 5)
    s1 = PlayerState(5, 0, 5)

    max_turns = 1000
    turns = 0
    while s0.lives() > 0 and s1.lives() > 0 and turns < max_turns:
        turns += 1

        # print("{} {} {}".format(s0.lives(), s0.bullets(), s0.remainingShields()))
        # print("{} {} {}".format(s1.lives(), s1.bullets(), s1.remainingShields()))

        a0 = p0.play(s0, s1)
        a1 = p1.play(s1, s0)

        # print("Player 0 plays {}".format(a0))
        # print("Player 1 plays {}".format(a1))

        applyActions(p0, p1, s0, s1, a0, a1)

    if turns >= max_turns or (s0.lives() == 0 and s1.lives() == 0):
        print("Draw")
        return -1
    elif s0.lives() > 0 and s1.lives() == 0:
        print("Player 0 wins")
        return 0
    elif s0.lives() == 0 and s1.lives() > 0:
        print("Player 1 wins")
        return 1
    else:
        print("what ?")
        print("turns=", turns)
        print("p0: lives={} bullets={} shields={}", s0.lives(), s0.bullets(), s0.remainingShields())
        print("p1: lives={} bullets={} shields={}", s1.lives(), s1.bullets(), s1.remainingShields())
        return -2

if __name__ == "__main__":
    p0 = Player(PlayerType.RANDOM, 0)
    p1 = Player(PlayerType.QLEARNER, 1)

    games = []
    for _ in range(1000):
        res = playGame(p0, p1)
        games.append(res)

    print(Counter(games))    

