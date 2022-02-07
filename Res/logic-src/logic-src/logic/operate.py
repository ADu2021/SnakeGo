import logging
from dataclasses import dataclass
from typing import List, TypedDict
from logic.infrastructure import Context, Operation, Snake
from logic.communication import ErrorType, PlayerError


class UndeterminableDirectionError:
    def __str__(self):
        return "occurs when snake length = 1 and try to destroy wall, unable to determine the direction"


class Graph:
    def __init__(self, bound, h, w):
        self.table = []
        self.kind = 0
        self.h = h
        self.w = w
        for i in range(h):
            r = []
            for j in range(w):
                r.append(0)
            self.table.append(r)
        for x, y in bound:
            self.table[x][y] = -1

    def check(self, c):
        for i in range(self.h):
            for j in range(self.w):
                if self.table[i][j] == c:
                    if i == 0 or j == 0 or i == self.h - 1 or j == self.w - 1:
                        return False
        return True

    def calc(self):
        for i in range(self.h):
            for j in range(self.w):
                self.floodfill(i, j)
        ret = []
        for k in range(1, self.kind + 1):
            if self.check(k):
                for i in range(self.h):
                    for j in range(self.w):
                        if self.table[i][j] == k:
                            ret.append((i, j))
        return ret

    def floodfill(self, x, y):
        if self.table[x][y] != 0:
            return
        self.kind = self.kind + 1
        self.table[x][y] = self.kind
        self.dfs(x, y, self.kind)

    def valid(self, x, y):
        return 0 <= x < self.w and 0 <= y < self.h

    def dfs(self, x, y, c):
        dx = [0, 0, 1, -1]
        dy = [1, -1, 0, 0]
        for i in range(4):
            tx, ty = x + dx[i], y + dy[i]
            if self.valid(tx, ty) and self.table[tx][ty] == 0:
                self.table[tx][ty] = c
                self.dfs(tx, ty, c)


@dataclass
class RoundInfo:
    ItemSnake = TypedDict("ItemSnake", {"item": int, "snake": int})
    ExtraLength = TypedDict("ExtraLength", {"snake": int, "el": int})

    gotten_item: List[ItemSnake]
    expired_item: List[ItemSnake]
    invalid_item: List[ItemSnake]
    extra_length: List[ExtraLength]

    def __init__(self):
        self.gotten_item = []
        self.expired_item = []
        self.invalid_item = []
        self.extra_length = []


class Controller:
    def __init__(self, ctx: Context):
        self.ctx = ctx
        self.map = ctx.get_map()
        self.player = 0
        self.next_snake = -1
        self.current_snake_list = []

    def round_preprocess(self, expire_time: int) -> RoundInfo:
        round_info = RoundInfo()
        old_extra_length = [snake.length_bank for snake in self.ctx.snake_list]
        tmp_item_list = self.map.item_list.copy()
        for item in tmp_item_list:
            if item.time <= self.ctx.turn - expire_time and item.gotten_time == -1:
                round_info.expired_item.append({"item": item.id, "snake": -1})
                self.map.delete_map_item(item.id)
            if item.time == self.ctx.turn:
                snake = self.map.snake_map[item.x][item.y]
                wall = self.map.wall_map[item.x][item.y]

                if snake >= 0:
                    item.gotten_time = self.ctx.turn
                    self.ctx.get_snake(snake).add_item(item)
                    self.ctx.game_map.item_list.remove(item)
                    round_info.gotten_item.append({"item": item.id, "snake": snake})
                # elif wall >= 0:
                #     self.ctx.game_map.item_list.remove(item)
                #     round_info.invalid_item.append({"item": item.id, "snake": snake})
                else:
                    self.map.item_map[item.x][item.y] = item.id
        for snake in self.ctx.snake_list:
            for item in snake.item_list:
                if self.ctx.turn - item.gotten_time > item.param:
                    snake.delete_item(item.id)
                    round_info.expired_item.append({"item": item.id, "snake": snake.id})
        round_info.extra_length = [{"snake": snake.id, "el": snake.length_bank}
                                   for i, snake in enumerate(self.ctx.snake_list) if
                                   old_extra_length[i] != snake.length_bank]
        return round_info

    def find_first_snake(self):
        for idx, snake in enumerate(self.ctx.snake_list):
            if snake.camp == self.player:
                return snake.id
        return -1

    def find_next_snake(self):
        for idx, (snake, dead) in enumerate(self.current_snake_list[self.next_snake + 1::]):
            if snake.camp == self.player and not dead:
                self.next_snake = self.next_snake + 1 + idx
                return
        self.next_snake = -1

    def next_player(self):
        self.player = self.ctx.current_player = 1 - self.ctx.current_player
        if self.player == 0:
            self.ctx.turn = 1 + self.ctx.turn
        self.next_snake = -1

    def delete_snake(self, s_id: int):
        self.ctx.delete_snake(s_id)
        temp = self.current_snake_list
        self.current_snake_list = [(i, i.id == s_id or dead) for (i, dead) in temp]

    def round_init(self):
        self.current_snake_list = [(i, False) for i in self.ctx.snake_list]
        self.find_next_snake()

    def apply(self, op: Operation):
        processed = self.apply_single(self.next_snake, op)
        logging.debug("Snake list:")
        for snake in self.ctx.snake_list:
            logging.debug(str(snake))
        self.find_next_snake()
        return processed

    def calc(self, coor: [(int, int)]) -> [(int, int)]:
        g = Graph(coor, self.map.length, self.map.width)
        return g.calc()

    def apply_single(self, snake: int, op: Operation):
        s, _ = self.current_snake_list[snake]
        idx_in_ctx = -1
        for idx, t in enumerate(self.ctx.snake_list):
            if s.id == t.id:
                idx_in_ctx = idx
        assert(idx_in_ctx != -1)
        idx_in_current = snake
        op.snake = s.id
        if op.type <= 4:  # move
            op.direction = op.type - 1
            op.type = 1
            ret = self.move(idx_in_ctx, idx_in_current, op.direction)
            extra_solid_area = ret[2]
            return {
                "basic": op,
                "dead_snake": ret[3],
                "should_solid": ret[0] == 'solidify',
                "extra_solid_area": [{"x": coord[0], "y": coord[1]} for coord in extra_solid_area],
                "got_item": ret[1],
                "extra_length": 0 if op.snake in ret[3] else self.ctx.get_snake(op.snake).length_bank,
                "new_snake_id": -1
            }
        if op.type == 5:
            op.type = 2
            if len(self.ctx.snake_list[idx_in_ctx].item_list) == 0:
                raise PlayerError(ErrorType.IA, self.ctx.snake_list[idx_in_ctx].camp, "No item!!!")
            elif self.ctx.snake_list[idx_in_ctx].item_list[0].type == 2:
                item = self.ctx.snake_list[idx_in_ctx].item_list.pop(0)
                op.item_id = item.id
                ret = self.fire(idx_in_ctx, idx_in_current)
                return {
                    "basic": op,
                    "dead_snake": ret[2],
                    "should_solid": False,
                    "extra_solid_area": [],
                    "got_item": -1,
                    "extra_length": 0 if len(ret[2]) > 0 else self.ctx.snake_list[idx_in_ctx].length_bank,
                    "new_snake_id": ret[3]
                }
        if op.type == 6:
            op.type = 3
            ret = self.split(idx_in_ctx, idx_in_current)
            return {
                "basic": op,
                "dead_snake": ret[2],
                "should_solid": False,
                "extra_solid_area": [],
                "got_item": -1,
                "extra_length": 0 if len(ret[2]) > 0 else self.ctx.get_snake(op.snake).length_bank,
                "new_snake_id": ret[3]
            }

    def get_item(self, snake: Snake, item_id: int) -> None:
        item = self.map.get_map_item(item_id)
        item.gotten_time = self.ctx.turn
        if item.type == 0:
            snake.length_bank += item.param
        else:
            snake.add_item(item)
        self.map.delete_map_item(item_id)

    def move(self, idx_in_ctx: int, idx_in_current: int, direction: int):
        dx = [1, 0, -1, 0]
        dy = [0, 1, 0, -1]
        snake = self.ctx.snake_list[idx_in_ctx]
        snake_id = snake.id
        auto_grow = self.ctx.turn <= self.ctx.auto_growth_round and snake.camp == snake.id
        self.ctx.delete_snake(snake_id)
        coor = snake.coor_list
        x, y = coor[0][0] + dx[direction], coor[0][1] + dy[direction]
        if len(coor) == 1:
            new_coor = [(x, y)]
        else:
            new_coor = [(x, y)] + coor[:-1]
        items_get = -1
        if x < 0 or x >= self.ctx.game_map.length or y < 0 or y >= self.ctx.game_map.width \
                or self.map.wall_map[x][y] != -1:
            self.delete_snake(snake_id)
            return ["invalid move", items_get, [], [snake_id]]

        if (len(coor) > 2 or (len(coor) == 2 and (auto_grow or snake.length_bank))) and (x, y) == coor[1]:
            raise PlayerError(ErrorType.IA, snake.camp, "DON'T TURN BACK!")

        if auto_grow:
            new_coor = new_coor + [coor[-1]]
        elif snake.length_bank:
            snake.length_bank = snake.length_bank - 1
            new_coor = new_coor + [coor[-1]]
        snake.coor_list = new_coor

        if self.map.item_map[x][y] != -1:
            items_get = self.map.item_map[x][y]
            self.get_item(snake, self.map.item_map[x][y])

        for i in range(len(new_coor)):
            if i == 0:
                continue
            if x == new_coor[i][0] and y == new_coor[i][1]:
                dead_snake = [snake_id]
                solid_coor = new_coor[:i]
                extra_solid = self.calc(solid_coor)
                for coor in new_coor[i:]:
                    if coor in extra_solid:
                        solid_coor.append(coor)
                        extra_solid.remove(coor)
                tmp_solid = extra_solid.copy()
                for coor in tmp_solid:
                    if self.map.snake_map[coor[0]][coor[1]] != -1:
                        dead_snake.append(self.map.snake_map[coor[0]][coor[1]])
                        self.delete_snake(dead_snake[-1])
                    # if self.map.item_map[coor[0]][coor[1]] != -1:
                    #     self.ctx.game_map.delete_map_item(self.map.item_map[coor[0]][coor[1]])
                    # if self.map.wall_map[coor[0]][coor[1]] == snake.camp:
                    #     extra_solid.remove(coor)
                self.map.set_wall(solid_coor, self.player, 1)
                self.map.set_wall(extra_solid, self.player, 1)
                self.delete_snake(snake_id)
                extra_solid.extend(solid_coor)
                extra_solid = sorted(extra_solid)
                return ["solidify", items_get, extra_solid, dead_snake]

        if self.map.snake_map[x][y] != -1:
            r_id = self.map.snake_map[x][y]
            r = self.ctx.get_snake(r_id)
            self.delete_snake(snake_id)
            return ["invalid move", items_get, [], [snake_id]]
            # if r.camp == self.player:
            #     self.delete_snake(snake_id)
            #     return ["invalid move", items_get, [], [snake_id]]
            # elif r.get_len() >= snake.get_len():
            #     self.delete_snake(snake_id)
            #     return ["invalid move", items_get, [], [snake_id]]
            # else:
            #     for i in range(idx_in_ctx):
            #         if r_id == self.ctx.snake_list[i].id:
            #             idx_in_ctx -= 1
            #             break
            #     self.delete_snake(r_id)
            #     self.ctx.add_snake(snake, idx_in_ctx)  # restore the deleted snake
            #     return ["success eat", items_get, [], [r_id]]

        self.ctx.add_snake(snake, idx_in_ctx)
        return ["success move", items_get, [], []]

    def split(self, idx_in_ctx: int, idx_in_current: int):
        def generate(pos, its, player, length_bank, index) -> int:
            ret = Snake(pos, its, player, -1)
            ret.length_bank = length_bank
            self.ctx.add_snake(ret, index)
            return ret.id

        snake = self.ctx.snake_list[idx_in_ctx]
        coor = snake.coor_list
        items = snake.item_list

        items_get = -1

        if self.ctx.get_snake_count(snake.camp) >= 4:
            raise PlayerError(ErrorType.IA, snake.camp, "Split too much!!!")

        if len(coor) <= 1:
            raise PlayerError(ErrorType.IA, snake.camp, "Split length too short!!!")

        head = coor[:(len(coor) + 1) // 2]
        tail = coor[(len(coor) + 1) // 2:]
        tail = tail[::-1]

        h_item = []
        t_item = []

        for item in items:
            if item.type == 0:
                t_item.append(item)
            elif item.type == 1:
                continue
            else:
                h_item.append(item)

        snake.coor_list = head
        snake.item_list = h_item
        tid = generate(tail, t_item, self.player, snake.length_bank, idx_in_ctx + 1)
        snake.length_bank = 0
        return ["success split", items_get, [], tid]

    def fire(self, idx_in_ctx: int, idx_in_current: int):
        snake = self.ctx.snake_list[idx_in_ctx]
        coor = snake.coor_list

        items_get = -1

        if len(coor) <= 1:
            raise PlayerError(ErrorType.IA, snake.camp, "Fire length too short!!!")

        x1, y1 = coor[0]
        x2, y2 = coor[1]
        dx, dy = x1 - x2, y1 - y2
        walls = []

        while self.map.length > x1 + dx >= 0 and self.map.width > y1 + dy >= 0:
            x1, y1 = (x1 + dx, y1 + dy)
            walls = [(x1, y1)] + walls

        self.map.set_wall(walls, -1, -1)
        return ["success fire", items_get, [], -1]
