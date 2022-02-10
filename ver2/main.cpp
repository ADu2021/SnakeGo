#include "adk.hpp"
#include <cmath>

bool is_valid_operation( const Operation& op, const Context& ctx, const Snake& snake_to_operate )
{
	static int dx[] = { 0, 1, 0, -1, 0 }, dy[] = { 0, 0, 1, 0, -1 };
	if(op == OP_RAILGUN)
	{
		if ( snake_to_operate.railgun_item.id < 0 )
			return false;
		if ( snake_to_operate.coord_list.size() < 2 )
			return false;
		return true;
	}
	if(op == OP_SPLIT)
	{
		if ( ctx.my_snakes().size() == SNAKE_LIMIT )
			return false;
		if ( snake_to_operate.coord_list.size() < 2 )
			return false;
		return true;
	}
	else{
		
		//蛇头到达位置
		int t_x = snake_to_operate[0].x + dx[op.type];
		int t_y = snake_to_operate[0].y + dy[op.type];

		//超出边界
		if ( t_x >= ctx.length() || t_x < 0 || t_y >= ctx.width() || t_y < 0 )
		{
			return false;
		}

		//撞墙
		if ( ctx.wall_map()[t_x][t_y] != -1 )
		{
			return false;
		}

		//撞非本蛇
		if ( ctx.snake_map()[t_x][t_y] != -1 && ctx.snake_map()[t_x][t_y] != snake_to_operate.id )
		{
			return false;
		}

		//回头撞自己
		if ( ( snake_to_operate.length() > 2 ||
			   ( snake_to_operate.length() == 2 && snake_to_operate.length_bank > 0 ) ) &&
			 snake_to_operate[1].x == t_x && snake_to_operate[1].y == t_y )
		{
			return false;
		}
	}
	return true;
}

void bfs(const Context& ctx, int ret[16][16], int last[16][16], int dist[16][16])
//ret是自己蛇的id，last存着这个位置的前驱，dist是距离
{
	static int dx[] = { 0, 1, 0, -1, 0 }, 
			   dy[] = { 0, 0, 1, 0, -1 };
	int no_dir[5] = {0,0,0,0};//回头撞自己
	std::queue<Coord> q;
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 16; j++)
			ret[i][j] = -1;
	for(int i = 0, len = ctx.my_snakes().size(); i < len; ++i)
	{
		Coord tmp = ctx.my_snakes()[i].coord_list[0];
		ret[tmp.x][tmp.y] = i;
		dist[tmp.x][tmp.y] = 0;
		q.push(tmp);
		for(int j = 1, len2 = ctx.my_snakes()[i].length(), len_bk = ctx.my_snakes()[i].length_bank; j < len2; ++j)
		{
			Coord tmp1 = ctx.my_snakes()[i].coord_list[j];
			ret[tmp1.x][tmp1.y] = -1 -len2 +j -len_bk;
		}
		for(int j = 1; j <= 4; j++)
		{
			if((ctx.my_snakes()[i].length() > 2 || (ctx.my_snakes()[i].length() == 2 && ctx.my_snakes()[i].length_bank > 0)) 
			&& (ctx.my_snakes()[i].coord_list[0] + Coord{dx[j], dy[j]} == ctx.my_snakes()[i].coord_list[1]))
			{
				no_dir[i] = j;
				break;
			}
		}
	}
	for(int i = 0, len = ctx.opponents_snakes().size(); i < len; ++i)
		for(int j = 0, len2 = ctx.opponents_snakes()[i].coord_list.size(), len_bk = ctx.opponents_snakes()[i].length_bank; j < len2; ++j)
		{
			Coord tmp = ctx.opponents_snakes()[i].coord_list[j];
			ret[tmp.x][tmp.y] = -1 -len2 +j -len_bk;
			//todo : 这里我认为对方的蛇所在的位置是不能走的，但是可以考虑到对方的蛇在几回合之后还是否会占据那个格子
			// -2 代表这里不能走
		}
	int counter = 0;
	while(!q.empty())
	{
		++counter;
		//fprintf(stderr,"%d\n",counter);
		Coord tmp = q.front();
		q.pop();
		for(int i = 1; i <= 4; ++i)
		{
			Coord next = tmp + Coord(dx[i], dy[i]);
			if(next.x < 0 || next.x >= 16 || next.y < 0 || next.y >= 16)
				continue;
			if ( ctx.wall_map()[next.x][next.y] != -1 )
				continue;
			if(dist[tmp.x][tmp.y] == 0 && i == no_dir[ret[tmp.x][tmp.y]] + 1)
				continue;
			if(ret[next.x][next.y] <= -2)
			{
				if(ret[next.x][next.y] + dist[tmp.x][tmp.y] + 1 >= 0)
				{
					ret[next.x][next.y] = ret[tmp.x][tmp.y];
					dist[next.x][next.y] = dist[tmp.x][tmp.y] + 1;
					last[next.x][next.y] = i;
					q.push(next);
				}
				else
					continue;
			}
			else if(ret[next.x][next.y] == -1)
			{
				ret[next.x][next.y] = ret[tmp.x][tmp.y];
				dist[next.x][next.y] = dist[tmp.x][tmp.y] + 1;
				last[next.x][next.y] = i;
				q.push(next);
			}
		}
	}

	fprintf(stderr,"round:%d\n",ctx.current_round());
	for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < 16; j++)
			fprintf(stderr,"%d ",ret[i][j]);
		fprintf(stderr,"\n");
	}

	return;
}

void bfs_bysnake(const Context& ctx, const Snake& snake, int ret[16][16], int last[16][16], int dist[16][16])
//ret是自己蛇的id，last存着这个位置的前驱，dist是距离
{
	static int dx[] = { 0, 1, 0, -1, 0 }, dy[] = { 0, 0, 1, 0, -1 };
	int no_dir[5] = {0,0,0,0};//回头撞自己
	std::queue<Coord> q;
	for(int i = 0, len = ctx.my_snakes().size(); i < len; ++i)
	{
		Coord tmp = ctx.my_snakes()[i].coord_list[0];
		if(ctx.my_snakes()[i] == snake)
		{
			ret[tmp.x][tmp.y] = i;
			dist[tmp.x][tmp.y] = 0;
			q.push(tmp);
		}
		for(int j = 0, len2 = ctx.my_snakes()[i].length(), len_bk = ctx.my_snakes()[i].length_bank; j < len2; ++j)
		{
			if(ctx.my_snakes()[i] == snake && j == 0)
				continue;
			Coord tmp1 = ctx.my_snakes()[i].coord_list[j];
			ret[tmp1.x][tmp1.y] = -1 -len2 +j -len_bk;
		}
		for(int j = 1; j <= 4; j++)
		{
			if((ctx.my_snakes()[i].length() > 2 || (ctx.my_snakes()[i].length() == 2 && ctx.my_snakes()[i].length_bank > 0)) 
			&& (ctx.my_snakes()[i].coord_list[0] + Coord{dx[j], dy[j]} == ctx.my_snakes()[i].coord_list[1]))
			{
				no_dir[i] = j;
				break;
			}
		}
	}
	for(int i = 0, len = ctx.opponents_snakes().size(); i < len; ++i)
		for(int j = 0, len2 = ctx.opponents_snakes()[i].coord_list.size(), len_bk = ctx.opponents_snakes()[i].length_bank; j < len2; ++j)
		{
			Coord tmp = ctx.opponents_snakes()[i].coord_list[j];
			ret[tmp.x][tmp.y] = -1 -len2 +j -len_bk;
			//todo : 这里我认为对方的蛇所在的位置是不能走的，但是可以考虑到对方的蛇在几回合之后还是否会占据那个格子
			// -2 代表这里不能走
		}
	int counter = 0;
	while(!q.empty())
	{
		++counter;
		//fprintf(stderr,"%d\n",counter);
		Coord tmp = q.front();
		q.pop();
		for(int i = 1; i <= 4; ++i)
		{
			Coord next = tmp + Coord(dx[i], dy[i]);
			if(next.x < 0 || next.x >= 16 || next.y < 0 || next.y >= 16)
				continue;
			if ( ctx.wall_map()[next.x][next.y] != -1 )
				continue;
			if(dist[tmp.x][tmp.y] == 0 && i == no_dir[ret[tmp.x][tmp.y]] + 1)
				continue;
			if(ret[next.x][next.y] <= -2)
			{
				if(ret[next.x][next.y] + dist[tmp.x][tmp.y] + 1 >= 0)
				{
					ret[next.x][next.y] = ret[tmp.x][tmp.y];
					dist[next.x][next.y] = dist[tmp.x][tmp.y] + 1;
					last[next.x][next.y] = i;
					q.push(next);
				}
				else
					continue;
			}
			else if(ret[next.x][next.y] == -1)
			{
				ret[next.x][next.y] = ret[tmp.x][tmp.y];
				dist[next.x][next.y] = dist[tmp.x][tmp.y] + 1;
				last[next.x][next.y] = i;
				q.push(next);
			}
		}
	}
}

Operation OP[7] = {OP_RIGHT, OP_RIGHT, OP_UP, OP_LEFT, OP_DOWN, OP_RAILGUN, OP_SPLIT};
Operation my_op[5];
int my_op_dist[5];

Operation i_just_wanna_eat( const Snake& snake_to_operate, const Context& ctx, const OpHistory& op_list )
{
	static int dx[] = { 0, 1, 0, -1, 0 }, dy[] = { 0, 0, 1, 0, -1 };
	static int ret[16][16];
	static int last[16][16];
	static int dist[16][16];
	static int cnt = 0;
	static bool mark[5];
	

	if ( snake_to_operate.id == ctx.my_snakes()[0].id )
	{
		cnt = 0;
		for(int i = 0; i < 4; i++)
			mark[i] = false, my_op_dist[i] = -1;
		
		bfs(ctx, ret, last, dist);
		
		for ( int i = 0; i < ctx.item_list().size(); i++ )
		{
			//没有搜到合适的蛇
			if( ret[ctx.item_list()[i].x][ctx.item_list()[i].y] < 0)
				continue;

			//搜到了合适的蛇，但已经有任务了
			if(mark[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]])
				continue;

			//道具已经消失
			if ( ctx.item_list()[i].time + ITEM_EXPIRE_LIMIT < ctx.current_round() )
				continue;

			//todo:这里的判断条件要改一下：我不一定要落在那16回合内
			if ( ctx.item_list()[i].time <= ctx.current_round() + dist[ctx.item_list()[i].x][ctx.item_list()[i].y] &&
				 ctx.item_list()[i].time + 16 > ctx.current_round() + dist[ctx.item_list()[i].x][ctx.item_list()[i].y] )
			{
				//找最近的那个
				if(!(my_op_dist[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]] == -1 ||
					my_op_dist[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]] > dist[ctx.item_list()[i].x][ctx.item_list()[i].y]))
					continue;

				int dir = last[ctx.item_list()[i].x][ctx.item_list()[i].y];
				Coord now = { ctx.item_list()[i].x, ctx.item_list()[i].y };

				//刚好在头上，不管
				if(now == ctx.my_snakes()[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]].coord_list[0])
					continue;

				fprintf(stderr,"route:\n");

				while(now != ctx.my_snakes()[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]].coord_list[0])
				{
					fprintf(stderr, "%d %d\n", now.x, now.y);
					dir = last[now.x][now.y];
					now = now - Coord(dx[dir], dy[dir]);
				}
				
				my_op[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]] = OP[dir];
				mark[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]] = true;
				my_op_dist[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]] = dist[ctx.item_list()[i].x][ctx.item_list()[i].y];
			}
		}
	}

	if(mark[cnt])
	{
		fprintf(stderr,"by bfs\n\n");
		++cnt;
		return my_op[cnt-1];
	}
	else
	{
		fprintf(stderr,"by default\n\n");
		++cnt;
		return make_your_decision(snake_to_operate, ctx, op_list);
	}
}

Operation make_your_decision( const Snake& snake_to_operate, const Context& ctx, const OpHistory& op_list )
{
	//若该蛇有道具，执行融化射线操作
	//todo: 不要这么随意的使用道具
	if ( snake_to_operate.railgun_item.id != -1 )
	{
		return OP_RAILGUN;
	}

	//若为该玩家操控的首条蛇且长度超过10且蛇数少于4则分裂
	if ( snake_to_operate == ctx.my_snakes()[0] )
	{
		//若该蛇长度超过10
		if ( snake_to_operate.length() >= 10 )
		{
			//若蛇的数量少于4
			if ( ctx.my_snakes().size() < 4 )
				return OP_SPLIT;
		}
	}

	Operation direction[4] = { OP_RIGHT, OP_UP, OP_LEFT, OP_DOWN };

	int dx[4] = { 1, 0, -1, 0 };
	int dy[4] = { 0, 1, 0, -1 };
	//确定合法移动方向
	//-1代表移动非法，0代表正常移动，1代表移动触发固化
	int move[4] = { 0, 0, 0, 0 };
	for ( int dir = 0; dir < 4; dir++ )
	{
		//蛇头到达位置
		int t_x = snake_to_operate[0].x + dx[dir];
		int t_y = snake_to_operate[0].y + dy[dir];

		//超出边界
		if ( t_x >= ctx.length() || t_x < 0 || t_y >= ctx.width() || t_y < 0 )
		{
			move[dir] = -1;
			continue;
		}

		//撞墙
		if ( ctx.wall_map()[t_x][t_y] != -1 )
		{
			move[dir] = -1;
			continue;
		}

		//撞非本蛇
		if ( ctx.snake_map()[t_x][t_y] != -1 && ctx.snake_map()[t_x][t_y] != snake_to_operate.id )
		{
			move[dir] = -1;
			continue;
		}

		//回头撞自己
		if ( ( snake_to_operate.length() > 2 ||
			   ( snake_to_operate.length() == 2 && snake_to_operate.length_bank > 0 ) ) &&
			 snake_to_operate[1].x == t_x && snake_to_operate[1].y == t_y )
		{
			move[dir] = -1;
			continue;
		}

		//移动导致固化
		if ( ctx.snake_map()[t_x][t_y] == snake_to_operate.id )
		{
			move[dir] = 1;
			continue;
		}
	}
	//玩家操控的首条蛇朝向道具移动
	if ( snake_to_operate == ctx.my_snakes()[0] )
	{
		for ( int i = 0; i < ctx.item_list().size(); i++ )
		{
			//道具已经消失
			if ( ctx.item_list()[i].time + ITEM_EXPIRE_LIMIT < ctx.current_round() )
				continue;

			//计算蛇头与道具的距离
			int distance = abs( ctx.item_list()[i].x - snake_to_operate[0].x ) +
						   abs( ctx.item_list()[i].y - snake_to_operate[0].y );

			//忽略其他因素理论上能吃到该道具
			if ( ctx.item_list()[i].time <= ctx.current_round() + distance &&
				 ctx.item_list()[i].time + 16 > ctx.current_round() + distance )
			{
				for ( int dir = 0; dir < 4; dir++ )
				{
					//移动不合法或固化
					if ( move[dir] != 0 )
						continue;

					//朝向道具方向移动
					int t_x = snake_to_operate[0].x + dx[dir];
					int t_y = snake_to_operate[0].y + dy[dir];
					int dis = abs( ctx.item_list()[i].x - t_x ) + abs( ctx.item_list()[i].y - t_y );
					if ( dis <= distance )
					{
						return direction[dir];
					}
				}
			}
		}
	}

	//其他蛇优先固化，或朝向蛇尾方向移动
	else
	{
		//计算蛇头与蛇尾的距离
		int distance = abs( snake_to_operate.coord_list.back().x - snake_to_operate.coord_list[0].x ) +
					   abs( snake_to_operate.coord_list.back().y - snake_to_operate.coord_list[0].y );
		for ( int dir = 0; dir < 4; dir++ )
		{
			//移动不合法或固化
			if ( move[dir] == -1 )
				continue;

			if ( move[dir] == 1 )
			{
				return direction[dir];
			}

			//朝向蛇尾方向移动
			int t_x = snake_to_operate.coord_list[0].x + dx[dir];
			int t_y = snake_to_operate.coord_list[0].y + dy[dir];
			int dis =
				abs( snake_to_operate.coord_list.back().x - t_x ) + abs( snake_to_operate.coord_list.back().y - t_y );
			if ( dis <= distance )
			{
				return direction[dir];
			}
		}
	}

	//以上操作均未正常执行，则以如下优先顺序执行该蛇移动方向
	//向空地方向移动>向固化方向移动>向右移动
	for ( int dir = 0; dir < 4; dir++ )
	{
		if ( move[dir] == 0 )
		{
			return direction[dir];
		}
	}
	for ( int dir = 0; dir < 4; dir++ )
	{
		if ( move[dir] == 1 )
		{
			return direction[dir];
		}
	}
	if ( ( snake_to_operate.length() > 2 || ( snake_to_operate.length() == 2 && snake_to_operate.length_bank > 0 ) ) &&
		 snake_to_operate[1].x == snake_to_operate[0].x + dx[0] &&
		 snake_to_operate[1].y == snake_to_operate[0].y + dy[0] )
		return OP_LEFT;
	else
		return OP_RIGHT;
}

void game_over( int gameover_type, int winner, int p0_score, int p1_score )
{
	fprintf( stderr, "%d %d %d %d", gameover_type, winner, p0_score, p1_score );
}

int main( int argc, char** argv ) { SnakeGoAI start( argc, argv ); }