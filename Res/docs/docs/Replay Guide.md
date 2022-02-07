
##回放文件格式简介

Saiblo网站的对局支持下载回放文件，回放文件格式为json，包含以下几个字段：

- `game_config`字段，代表游戏的配置信息，包括地图长宽`length`/`width`、最大回合数`max_round`、随机数种子`random_seed`。
```json
{ "length": 16, "width": 16, "max_round": 512, "random_seed": 1642262138104 }
```
- `item_list`字段，代表游戏开始时生成的道具列表，其中道具的每个字段与`Item`类（见ADK User Guide）中的成员一一对应。
```json
    [
        { "x": 7, "y": 6, "time": 3, "type": 0, "param": 5 },
        { "x": 8, "y": 6, "time": 16, "type": 2, "param": 512 },
        ...
    ]
```
- `round_info`字段，代表每回合开始时的信息，一回合的信息格式如下：

```json
    {
      "gotten_item": [            // 此回合直接生成在蛇身上的道具。item和snake均为对应的id
        {"item": 0, "snake": 1},
        {"item": 2, "snake": 3}
      ],
      "expired_item": [           // 此回合过期的道具，若为地图道具则snake为-1。
        {"item": 1, "snake": 2},
        {"item": 3, "snake": 5}
      ],
      "invalid_item": [],         // 此回合应无效化道具，弃用字段。
      "extra_length": [           // 此回合开始时因为某种原因（直接刷新的长度道具）而让长度银行发生变化的信息，el代表新的长度银行值
        {"snake": 1, "el": 5}
      ]
    }
```

- `operations`字段，代表每个回合的操作以及其附加信息，一回合的信息格式如下：

```json
    {
      // 原操作，type代表类型，snake代表蛇编号，direction代表方向，item_id代表道具编号。
      "basic": { "type": 1, "snake": 0, "direction": 0, "item_id": -1 },
      
      // 以下为移动操作所需要的附加信息
      "dead_snake": [1,2,3], // 因为本操作而消失的蛇的id列表。
      "should_solid": true,  // 执行本操作的蛇是否固化。
      "extra_solid_area": [  // 执行本操作的蛇固化的区域，按x、y升序排序。
          { "x": 2, "y": 1 },
          { "x": 2, "y": 2 },
          { "x": 3, "y": 1 },
          { "x": 3, "y": 2 },
      ],
      "got_item": 4,         // 执行本操作的蛇操作后吃到的道具。-1表示不存在。
      "extra_length": 1,     // “长度银行”剩余长度。
      
      // 以下为分裂道具所需要的附加信息
      "new_snake_id": 12     // 分裂产生的新蛇的id，没有则为-1。
    }
```

- `end_info`字段，代表结束信息。`type`为结束类型，`winner`代表获胜玩家编号，`score`代表两个玩家的得分，第一个值为`0`号玩家得分，第二个值为`1`号玩家得分，`err`代表错误信息。
```json
"end_info": { "type": "NORMAL", "winner": 0, "score": [71, 47], "err": null }
```