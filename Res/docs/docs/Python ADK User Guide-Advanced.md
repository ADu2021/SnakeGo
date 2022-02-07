# ADK User Guide-Advanced

> 建议在阅读本进阶版指南前，首先阅读Lite版指南。

[toc]
## 通讯部分
本ADK按照AI与逻辑之间的通讯协议（见`AI-Protocol.md`)提供了数据的收发功能，具体实现于`Client`类中。
### 初始化客户端
```python
c = adk.Client()
```
客户端初始化时，会检查运行时指定的命令行参数，判断是否使用`Socket`与本地调试工具进行连接，具体参数为`--local/-l --port=/-p=`，第二个参数等号后接指定端口号；若不带参数则默认使用`stdio`与评测机进行交互。
### 接收数据
`Client`类中的`fetch_data`方法用于从评测机接收数据，根据当前所处状态读取所需数据，状态与数据的对应如下：
- 状态`__state__ = 0`: 此时接收的数据是游戏对局的配置信息，返回四个整数，包括地图长宽、回合上限、当前玩家编号
- 状态`__state__ = 1`: 此时接收的数据是游戏生成的道具列表，返回一个`Item`类的列表，类型内容见下。
- 状态`__state__ = 2`: 此时进入游戏进行阶段，接收的数据为己方、本方的操作或是游戏结束信息，当前为玩家回合时，待玩家发送操作后若正常则会重新发回该操作，否则会接收到游戏结束信息；对手回合则可获取对手操作或是游戏结束信息。正常操作会返回只含一个整数的列表，而错误信息则会返回含五个整数的列表，以-1开头，之后分别为错误类型、获胜玩家、双方得分。

状态的切换无需由玩家手动进行，第一次调用该方法获取配置信息后自动切换至状态1，再一次调用获取道具列表后自动进入状态2。

### 发送数据
`Client`类中的`send_data`方法用于向评测机发送数据，即当前蛇的操作。需要一个类型为整数的参数，代表当前操作的编号。若该参数不为1~6中的整数，该函数会发出异常。

### 交互流程
总体客户端的交互流程如下：
```python
    c = Client()                                         # 初始化客户端
    (length, width, max_round, player) = c.fetch_data()  # 获取游戏配置信息
    item_list = c.fetch_data()                           # 获取道具列表
    while PLAYING:                                       # 游戏进行
        if OVER_MAX_ROUND:                               # 到达回合上限
            RESULT = c.fetch_data()                      # 接收正常游戏结束信息
            break
        if PLAYER_TURN:                                  # 当前为玩家回合
            while NEXT_SNAKE_EXISTS:                     # 还有未进行操作的蛇
                c.send_data(OPERATION)                   # 发送操作
                RESULT = c.fetch_data()                  # 接收反馈
                if res[0] == -1:                         # 游戏提前结束
                    PLAYING = False
                    break
        else:                                            # 对手回合
            while NEXT_SNAKE_EXISTS:                     # 对手仍有未进行操作的蛇
                OPERATION = c.fetch_data()               # 获取操作
                if OPERATION[0] == -1:                   # 游戏提前结束
                    PLAYING = False
                    break
```

## 游戏状态维护

本ADK提供了游戏局面维护逻辑代码，可减少选手的代码负担，直接调用现成代码即可。

### 基础类型

#### Item 道具类

- `x: int` x坐标
- `y: int` y坐标
- `id: int` 道具编号
- `time: int` 道具生成的时间
- `type: int` 道具类型，`0`代表长度道具，`2`代表融化射线    PS: 长度道具不会被保存，直接存入长度银行
- `param: int` 道具参数，代表道具的有效时间
- `gotten_time: int` 道具被蛇获取的时间（若道具仍在地图中未被蛇获取，为-1）
- `item_num: int` 静态成员，用于分配道具编号
- `__init__(self, x, y, time, type, param)` 初始化道具，id会自动分配

#### GameConfig 游戏配置类

- `length: int` 地图的长度
- `width: int` 地图的宽度
- `max_round: int` 最大回合上限
- `random_seed: int` 随机数种子
- `__init__(self, length, width, max_round)` 初始化游戏配置

#### Snake 蛇类

- `coor_list: List[Tuple[int, int]]` 一个顺序（蛇头 -> 蛇尾）包含所有坐标的列表，列表中元素为包含两个`int`的元组，第一个为`x`坐标，第二个为`y`坐标
- `item_list: List[Item]` 当前蛇所有的道具列表
- `id: int` 蛇的编号
- `length_bank: int` 长度银行中保存的长度
- `camp: int` 蛇归属于玩家0/1
- `snake_num: int` 静态成员，用于分配蛇的编号
- `__init__(self, coor_list, item_list, camp, id=-1)` 初始化道具，若无指定id会自动分配
- `get_len() -> int ` 返回当前蛇的长度
- `add_item(item: Item)` 为当前蛇增添道具`item`，若为长度道具则直接加入长度银行
- `get_item(id: int)` 获取当前蛇的编号为`id`的道具，若无返回`None`
- `delete_item(id: int)` 删除当前蛇编号为`id`的道具

#### Map 地图类

- `wall_map: List[List[int]]` -1 / 0 / 1 代表无墙 / 0号选手的墙 / 1号选手的墙

- `snake_map: List[List[int]]` -1 / id 代表无 / id号蛇占据

- `item_map: List[List[int]]` -1 / id 代表无 / id号道具占据

- `item_list: List[Item]`现存所有道具的列表

- `length: int` 地图长度

- `width: int` 地图宽度

- `__init__(self, item_list: List[Item], config: GameConfig)`  

  ​	根据游戏配置与生成的道具列表初始化地图，并设置蛇起始位置`(0, width - 1)`及`(length - 1, 0)`

- `get_map_item(id: int) -> Item` 返回当前编号为id的道具

- `set_wall(coor_list: List[Tuple[int, int]]), camp: int, type: int)`  

  ​	`type == 1`时将`coor_list`中所有坐标设为玩家`camp`的墙，`type == -1`时将`coor_list`中的墙摧毁

- `add_map_item(item: Item)` 将道具`item`加到地图中
- `delete_map_item(id: int)` 删除地图中的编号为`id`的道具
- `add_map_snake(coor_list:List[Tuple[int, int]], id: int)`  将编号为`id`的蛇加入地图中，坐标区域为`coor_list`
- `delete_map_item(coor_list:List[Tuple[int, int]])` 删除地图中原来坐标区域为`coor_list`的蛇

#### Context 上下文类

- `snake_list: List[Snake]` 包含现存所有蛇，同一个玩家的蛇按照操作顺序排序。
- `game_map: Map` 现在的游戏地图，为自定义类`Map`，具体内容见后。
- `turn: int` 当前回合数
- `current_player: int` 当前玩家编号
- `auto_growth_round: int` 自动增长回合数，默认为8
- `max_round: int` 最大回合数
- `__init__(self, config: GameConfig)`  根据游戏配置初始化上下文，并将初始两条蛇加入`snake_list`
- `get_map() -> Map` 返回当前地图
- `get_snake_count(camp: int) -> int` 返回当前`camp`选手蛇的数量
- `get_snake(id: int) -> Snake` 返回当前编号为id的蛇
- `add_snake(snake:Snake, index: int)` 在`snake_list`下标为`index`处插入新蛇，并加入地图中
- `delete_snake(id: int) -> int` 删除编号为`id`的蛇

#### Graph 固化计算

该类实际实现了floodfill用于计算固化围成的区域，使用方法如下：

`__init__(self, bound, h, w)` 初始化固化边界、地图长宽

`calc() -> List` 计算固化区域，返回固化区域（不包括蛇本身）

#### Operation 操作类 

#### （**该类已于ADK中弃用，操作请依照新的单个整数的形式**）

- `type: int` 操作类型 1移动，2融化，3分裂
- `snake: int` 蛇编号
- `direction: int` 方向 0 : x轴正向，1 : y轴正向, 2 : x轴负向, 3 : y轴负向，若无则为-1
- `item_id: int` 道具编号，若无则为-1

### 核心类Controller以及逻辑处理流程

`Controller`类为逻辑处理的核心，以下是其包含的成员属性与方法：

- `ctx: Context` 游戏上下文信息

- `map: Map` 游戏地图信息

- `player: int`当前玩家

- `next_snake: int` 待操作的蛇的编号，没有则为-1

- `current_snake_list: List[Tuple(Snake, boolean)]` 处理下一条蛇时的辅助数组，复制`ctx.snake_list`中的蛇，但删除死亡的蛇时通过元组中的布尔值标记是否死亡，不直接在列表中删除

- `__init__(self, ctx: Context)` 通过已初始化的上下文信息初始化Controller

- `round_preprocess()` 处理回合开始信息

- `find_first_snake() -> int` 查询初始的蛇（用于处理自动增长）

- `find_next_snake() -> int` 返回下一条需操作的蛇在`current_snake_list`数组中的编号 

- `next_player() -> int` 切换到下一个玩家

- `delete_snake(s_id: int)` 删除编号为`s_id`的蛇，会调用`ctx`中的`delete_snake`方法

- `round_init()` 玩家行动前的预处理，初始化`current_snake_list`并寻找第一条待操作的蛇

- `apply(op: int) ` `op`格式为与ADK交互的格式，单个整数，调用`apply_single`处理操作并寻找下一条蛇

- `apply_single(snake: int, op: int)` 处理在`current_snake_list`中下标为`snake`的蛇的操作

  根据不同操作，分别调用`move`/ `split`/ `fire`函数处理移动、分裂、融化

- `get_item(snake: Snake, item_id: int)` 为蛇`snake`增加编号为`item_id`的道具

#### 实例化Controller

游戏开始时，AI读入游戏配置、道具列表后可实例化`Controller`类型，调用构造函数，参数为实例化的`Context`对象。如下就是一段初始化代码：

```python
config = GameConfig(width=width, length=length, max_round=round_count)
ctx = Context(config=config)
ctx.game_map = Map(item_list, config=config)
controller = Controller(ctx)
```

#### 回合信息处理

当每个回合开始时，即轮到先手玩家进行操作前，需要调用`controller.round_preprocess`方法进行回合的预处理，处理生成在蛇上的道具、道具的过期等。

#### 玩家操作预处理

每个玩家开始操作前，还需调用`controller.round_init`方法预处理，查找第一条蛇。

#### 应用操作

玩家每进行一次操作，需要调用`controller.apply`方法处理当前操作并寻找下一条蛇。

#### 玩家切换

一个玩家结束后，需要调用`controller.next_player`方法切换到下一个玩家。

#### 框架示例

大致的逻辑处理流程框架如下：

```python
while playing:
    if controller.ctx.turn > max_round:               # 超出回合上限，游戏正常结束
        res = c.fetch_data() 
        sys.stderr.write(str(res[1:]))                # 获取结束信息并输出
        return
    if current_player == 0:
        controller.round_preprocess()
    controller.round_init()
    if player == current_player:                      # 玩家的回合
        while controller.next_snake != -1:            # 还有蛇需要操作
            op = ai.judge(...)                        # AI计算当前操作
            c.send(op)                                # 输出当前操作
            controller.apply(op)                      # 应用当前操作并寻找下一条蛇
            res = c.fetch_data()                      # 获取反馈（正常为原操作，若出现错误则为错误信息）
            if res[0] == -1:                          # 游戏提前结束
                playing = False
                sys.stderr.write(str(res[1:]))        # 错误信息输出
                break
        controller.next_player()                      # 切换玩家
        c.write_int(0)
    else:
       while True:
          if controller.next_snake == -1:             # 对手回合是否结束
              controller.next_player()
              break
          op = c.fetch_data()                         # 获取操作
          if op[0] == -1:                             # 出现错误，游戏提前结束
              playing = False
              sys.stderr.write(str(op[1:]))           # 错误信息输出
              break
          controller.apply(op)                        # 应用对手操作
    current_player = 1 - current_player               # 切换玩家
```