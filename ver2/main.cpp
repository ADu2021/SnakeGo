#include "adk.hpp"
#include <cmath>

const int MIN_DST_SIZE = 50;

#define fprintf //

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
			//不能回头撞自己
			if(dist[tmp.x][tmp.y] == 0 && i == no_dir[ret[tmp.x][tmp.y]])
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
	/*for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < 16; j++)
			fprintf(stderr,"%d ",ret[i][j]);
		fprintf(stderr,"\n");
	}*/

	return;
}

void bfs_bysnake(const Context& ctx, const Snake& snake, int ret[16][16], int last[16][16], int dist[16][16])
//ret是自己蛇的id，last存着这个位置的前驱，dist是距离
{
	//todo:重写
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
			if(dist[tmp.x][tmp.y] == 0 && i == no_dir[ret[tmp.x][tmp.y]])
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

bool is_dead_end_path(const Context& ctx, int block[16][16], Coord src, Coord dst)
// block是不让走的路线，通常是已经走过的path，不能block src
{
	static int dx[] = { 0, 1, 0, -1, 0 }, dy[] = { 0, 0, 1, 0, -1 };
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 16; j++)
			if(ctx.snake_map()[i][j] != -1) // todo:蛇可能移动
				block[i][j] = 1;
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 16; j++)
			if(ctx.wall_map()[i][j] != -1) // TODO:墙可能被融化 可能其它蛇来救一下
				block[i][j] = 1;
	
	bool vis[16][16];
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 16; j++)
			vis[i][j] = false;
	std::queue<Coord> q;
	q.push(dst);
	vis[dst.x][dst.y] = true;

	int src_size = 1;
	int dst_size = 1;

	while(!q.empty())
	{
		Coord tmp = q.front();
		q.pop();
		if(tmp == src)
			break;
		for(int i = 1; i <= 4; i++)
		{
			Coord next = tmp + Coord(dx[i], dy[i]);
			if(!in_zone(next))
				continue;
			if(block[next.x][next.y] == 1)
				continue;
			if(!vis[next.x][next.y])
			{
				q.push(next);
				vis[next.x][next.y] = true;
				++dst_size;
			}
		}
	}

	if(vis[src.x][src.y])
		return false;

	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 16; j++)
			vis[i][j] = false;
	q.push(src);
	vis[src.x][src.y] = true;
	while(!q.empty())
	{
		Coord tmp = q.front();
		q.pop();
		if(tmp == dst)
			break;
		for(int i = 1; i <= 4; i++)
		{
			Coord next = tmp + Coord(dx[i], dy[i]);
			if(!in_zone(next))
				continue;
			if(block[next.x][next.y] == 1)
				continue;
			if(!vis[next.x][next.y])
			{
				q.push(next);
				vis[next.x][next.y] = true;
				++src_size;
			}
		}
	}

	if(src_size > dst_size && dst_size < MIN_DST_SIZE) // todo : adjust MIN_DST_SIZE
		return true;
	return false;
}

int move_area(const Context& ctx, Coord dst)
{
	static int dx[] = { 0, 1, 0, -1, 0 }, dy[] = { 0, 0, 1, 0, -1 };
	std::queue<Coord> q;
	q.push(dst);
	int vis[16][16];
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 16; j++)
			vis[i][j] = 0;
	vis[dst.x][dst.y] = 1;
	int vis_cnt = 1;
	while(!q.empty())
	{
		Coord tmp = q.front();
		q.pop();
		for(int i = 1; i <= 4; i++)
		{
			Coord next = tmp + Coord(dx[i], dy[i]);
			if(!in_zone(next))
				continue;
			if(ctx.wall_map()[next.x][next.y] != -1)
				continue;
			if(ctx.snake_map()[next.x][next.y] != -1)
				continue;
			//todo:这里的snake_map应该想bfs那样优化一下?
			if(vis[next.x][next.y] == 0)
			{
				q.push(next);
				vis[next.x][next.y] = 1;
				vis_cnt++;
			}
		}
	}
	return vis_cnt;
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
	static int mark[5]; // 0 = no_task # 1 = eat # 5 = railgun # 6 = split

	if ( snake_to_operate.id == ctx.my_snakes()[0].id )
	{
		cnt = 0;
		for(int i = 0; i < 4; i++)
			mark[i] = 0, my_op_dist[i] = -1;
	}

	int snake_my_id = 0;
	for(int i = 0; i < ctx.my_snakes().size(); i++)
		if(ctx.my_snakes()[i].id == snake_to_operate.id)
			snake_my_id = i;
	
	//游戏前期，分裂收益较高
	if(ctx.current_round() <= 64)
	{
		//todo:考虑已分配eat任务和split的优先级关系
		if(ctx.my_snakes().size() <= 2 && snake_to_operate.length() >= 10)
		{
			my_op[snake_my_id] = OP_SPLIT;
			mark[snake_my_id] = 6;
		}
		
	}
	

	fprintf(stderr,"snake%d @ round%d\n",snake_to_operate.id,ctx.current_round());
	if ( snake_to_operate.id == ctx.my_snakes()[0].id )
	{	
		bfs(ctx, ret, last, dist);
		
		for ( int i = 0; i < ctx.item_list().size(); i++ )
		{
			//没有搜到合适的蛇
			if( ret[ctx.item_list()[i].x][ctx.item_list()[i].y] < 0)
				continue;

			//搜到了合适的蛇，但已经有任务了，找最近的那个
			if((mark[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]] == 1) && 
			(!(my_op_dist[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]] == -1 ||
					my_op_dist[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]] >= dist[ctx.item_list()[i].x][ctx.item_list()[i].y])))
				continue;

			//道具已经消失
			if ( ctx.item_list()[i].time + ITEM_EXPIRE_LIMIT < ctx.current_round() )
				continue;

			//todo:这里的判断条件要改一下：我不一定要落在那16回合内
			if ( ctx.item_list()[i].time <= ctx.current_round() + dist[ctx.item_list()[i].x][ctx.item_list()[i].y] &&
				 ctx.item_list()[i].time + 16 > ctx.current_round() + dist[ctx.item_list()[i].x][ctx.item_list()[i].y] )
			{

				int dir = last[ctx.item_list()[i].x][ctx.item_list()[i].y];
				Coord now = { ctx.item_list()[i].x, ctx.item_list()[i].y };

				//刚好在头上，不管
				if(now == ctx.my_snakes()[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]].coord_list[0])
					continue;

				int block[16][16];
				for(int i = 0; i < 16; i++) for(int j = 0; j < 16; j++) block[i][j] = 0;

				//fprintf(stderr,"route:\n");

				while(now != ctx.my_snakes()[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]].coord_list[0])
				{
					//fprintf(stderr, "%d %d\n", now.x, now.y);
					dir = last[now.x][now.y];
					block[now.x][now.y] = 1;
					now = now - Coord(dx[dir], dy[dir]);
				}

				//避免食物陷阱
				if(is_dead_end_path(ctx,block,ctx.my_snakes()[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]].coord_list[0],{ctx.item_list()[i].x, ctx.item_list()[i].y}))
					continue;
				
				my_op[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]] = OP[dir];
				mark[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]] = 1;
				my_op_dist[ret[ctx.item_list()[i].x][ctx.item_list()[i].y]] = dist[ctx.item_list()[i].x][ctx.item_list()[i].y];
			}
		}

		//没有eat 任务的蛇
		for(int i = 0; i < ctx.my_snakes().size(); i++)
		{
			if(mark[i])
				continue;
			//RAILGUN
			if( ( ctx.my_snakes()[i].railgun_item.id >= 0 ) && ( ctx.my_snakes()[i].length() >= 2 ) )
			{
				Coord diff = ctx.my_snakes()[i][0] - ctx.my_snakes()[i][1];
				int wall_count = 0;
				for(Coord j = ctx.my_snakes()[i][0] + diff; in_zone(j); j = j + diff)
				{
					if(ctx.wall_map()[j.x][j.y] == ctx.current_player())
						wall_count--;
					else if(ctx.wall_map()[j.x][j.y] == (1 - ctx.current_player()))
						wall_count++;
				}
				if(wall_count > 0)
				{
					my_op[i] = OP_RAILGUN;
					mark[i] = 5;
				}
			}
		}

	}

	if(mark[cnt])
	{
		fprintf(stderr,"by bfs:snake %d do %d\n",snake_to_operate.id, my_op[cnt].type);
		++cnt;
		return my_op[cnt-1];
	}
	else
	{
		fprintf(stderr,"by default\n");
		++cnt;
		return make_your_decision(snake_to_operate, ctx, op_list);
	}
}

Operation make_your_decision( const Snake& snake_to_operate, const Context& ctx, const OpHistory& op_list )
{
	//若为该玩家操控的首条蛇且长度超过10且蛇数少于4则分裂
	/*if ( snake_to_operate == ctx.my_snakes()[0] )
	{
		//若该蛇长度超过10
		if ( snake_to_operate.length() >= 10 )
		{
			//若蛇的数量少于4
			if ( ctx.my_snakes().size() < 4 )
				return OP_SPLIT;
		}
	}
	*/

	Operation direction[4] = { OP_RIGHT, OP_UP, OP_LEFT, OP_DOWN };

	int dx[4] = { 1, 0, -1, 0 };
	int dy[4] = { 0, 1, 0, -1 };
	//确定合法移动方向
	//-1代表移动非法，0代表正常移动，1代表移动触发固化,-2代表合法但可能进入死路
	int move[4] = { 0, 0, 0, 0 };
	double seal_gain[4] = { 0, 0, 0, 0 };
	double max_seal_gain = 0; int max_seal_gain_dir = -1;

	static int block[16][16];

	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 16; j++)
			block[i][j] = 0;
				

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
			//计算收益
			int snake_map[16][16];
			for(int i = 0; i < 16; i++)	for(int j = 0; j < 16; j++)	snake_map[i][j] = 0;
			for(int i = 0; i < snake_to_operate.length(); i++)
			{
				snake_map[snake_to_operate[i].x][snake_to_operate[i].y] = 1;
				if(snake_to_operate[i] == Coord{t_x,t_y})
					break;
			}
			seal_gain[dir] = seal_count(snake_map) / (double)seal_expect(snake_to_operate);
			if(max_seal_gain_dir == -1 || seal_gain[dir] > max_seal_gain)
			{
				max_seal_gain = seal_gain[dir];
				max_seal_gain_dir = dir;
			}
			continue;
		}

		//移动进入死路
		if(move_area(ctx,Coord{t_x, t_y}) <= 10)
		{
			move[dir] = -2;
			continue;
		}
	}

	fprintf(stderr, "move: %d %d %d %d \n", move[0], move[1], move[2], move[3]);

	//尝试固化
	//todo:if(snake_to_operate.length() > f(x)),f(x)单调递减
	
	if(ctx.current_round() > 508)
	{
		if(max_seal_gain*seal_expect(snake_to_operate) > snake_to_operate.length()*0.4)
			return direction[max_seal_gain_dir];
	}
	else if(ctx.current_round() > 504)
	{
		if(ctx.my_snakes().size() >= 2 &&
		max_seal_gain*seal_expect(snake_to_operate) > snake_to_operate.length()*0.5)
			return direction[max_seal_gain_dir];
	}
	else if(ctx.current_round() > 500)
	{
		if(ctx.my_snakes().size() >= 3 &&
		max_seal_gain*seal_expect(snake_to_operate) > snake_to_operate.length()*0.6 && 
			max_seal_gain > 0.75)
			return direction[max_seal_gain_dir];
	}
	else
	{
		if((ctx.my_snakes().size() >= 3) && (snake_to_operate.length() >= 9)
		&& (max_seal_gain*seal_expect(snake_to_operate) > snake_to_operate.length()*0.7))
		{
			fprintf(stderr,"here%lf*%d=%lf \n",max_seal_gain,seal_expect(snake_to_operate),max_seal_gain*seal_expect(snake_to_operate));
			return direction[max_seal_gain_dir];
		}
	}

	//	if(snake_to_operate.length() >= 10 || (snake_to_operate.length() >= 5 && ctx.current_round() > 480))
	//		return direction[max_seal_gain_dir];

	//尝试分裂
	if(ctx.my_snakes().size() < 4 && snake_to_operate.length() >= 10)
	{
		//尾巴不进入死路
		if(move_area(ctx,snake_to_operate[snake_to_operate.length()-1]) > 10)
			return OP_SPLIT;	
	}
	/*
	//玩家操控的首条蛇朝向道具移动
	//if ( snake_to_operate == ctx.my_snakes()[0] )
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
	*/

	//其他蛇优先固化，或朝向蛇尾方向移动
	//else
	{
		//计算蛇头与蛇尾的距离
		int distance = abs( snake_to_operate.coord_list.back().x - snake_to_operate.coord_list[0].x ) +
					   abs( snake_to_operate.coord_list.back().y - snake_to_operate.coord_list[0].y );
		for ( int dir = 0; dir < 4; dir++ )
		{
			//移动不合法
			if ( move[dir] == -1 )
				continue;

			/*
			if ( move[dir] == 1 )
			{
				return direction[dir];
			}
			*/

			//朝向蛇尾方向移动,不固化
			int t_x = snake_to_operate.coord_list[0].x + dx[dir];
			int t_y = snake_to_operate.coord_list[0].y + dy[dir];
			int dis =
				abs( snake_to_operate.coord_list.back().x - t_x ) + abs( snake_to_operate.coord_list.back().y - t_y );
			if ( dis <= distance )
			{
				if(move[dir] == 0)
					return direction[dir];
			}
		}
	}

	//以上操作均未正常执行，则以如下优先顺序执行该蛇移动方向
	//向空地方向移动>分裂>向固化方向移动>向死路移动>向右移动
	for ( int dir = 0; dir < 4; dir++ )
	{
		if ( move[dir] == 0 )
		{
			return direction[dir];
		}
	}

	if(ctx.my_snakes().size() < 4 && snake_to_operate.length() >= 2)
		return OP_SPLIT;

	if(max_seal_gain_dir != -1)
		return direction[max_seal_gain_dir];

	for ( int dir = 0; dir < 4; dir++ )
		if ( move[dir] == -2 )
			return direction[dir];

	if ( ( snake_to_operate.length() > 2 || ( snake_to_operate.length() == 2 && snake_to_operate.length_bank > 0 ) ) &&
		 snake_to_operate[1].x == snake_to_operate[0].x + dx[0] &&
		 snake_to_operate[1].y == snake_to_operate[0].y + dy[0] )
		return OP_LEFT;
	return OP_RIGHT;
}

bool in_zone(const Coord& coord)
{
	if(coord.x < 0 || coord.x >= 16 || coord.y < 0 || coord.y >= 16)
		return false;
	return true;
}

int seal_expect(const Snake& snake)
{
	int length = snake.length() - 1;
	int tar_x = ((length / 2) + 2) / 2;
	int tar_y = (length / 2) + 2 - tar_x;
	return tar_x * tar_y;
}

int seal_square(const Context& ctx, const Snake& snake)		// 0:Impossible		1:Safe
// not completed
{
	int tar_x = (snake.length() >> 2);
	int tar_y = (snake.length() >> 1) - tar_x;

	bool mark = true;
	if(in_zone(snake[0] + Coord{tar_x, tar_y}))
	{
		for(int i = snake[0].x; i < snake[0].x + tar_x; i++)
		{
			if(ctx.wall_map()[i][snake[0].y] != -1 || ctx.snake_map()[i][snake[0].y] != -1)
			{
				if(i == snake[0].x)
					continue;
				mark = false;
				break;
			}
			if(ctx.wall_map()[i][snake[0].y + tar_y] != -1 || ctx.snake_map()[i][snake[0].y + tar_y] != -1)
			{
				mark = false;
				break;
			}
		}
		for(int j = snake[0].y; j < snake[0].y + tar_y; j++)
		{
			if(ctx.wall_map()[snake[0].x][j] != -1 || ctx.snake_map()[snake[0].x][j] != -1)
			{
				if(j == snake[0].y)
					continue;
				mark = false;
				break;
			}
			if(ctx.wall_map()[snake[0].x + tar_x][j] != -1 || ctx.snake_map()[snake[0].x + tar_x][j] != -1)
			{
				mark = false;
				break;
			}
		}
	}
	if(mark)
		return 1;
	else
		return 0;
	return 0;
}

int seal_tri(const Context& ctx, const Snake& snake)
{
	return 0;
}

int seal_count(int snake_map[16][16])
{
	static int dx[] = { 0, 1, 0, -1, 0 }, dy[] = { 0, 0, 1, 0, -1 };

	std::queue<Coord> q;
	for(int i = 0; i < 16; i++)
	{
		if(!q.empty())
			break;
		if(snake_map[i][0] == 0)
		{
			q.push(Coord{i, 0});
			snake_map[i][0] = 2;
		}
		if(!q.empty())
			break;
		if(snake_map[i][15] == 0)
		{
			q.push(Coord{i, 15});
			snake_map[i][15] = 2;
		}
	}
	for(int j = 0; j < 16; j++)
	{
		if(!q.empty())
			break;
		if(snake_map[0][j] == 0)
		{
			q.push(Coord{0, j});
			snake_map[0][j] = 2;
		}
		if(!q.empty())
			break;
		if(snake_map[15][j] == 0)
		{
			q.push(Coord{15, j});
			snake_map[15][j] = 2;
		}
	}
	while(!q.empty())
	{
		Coord cur = q.front();
		q.pop();
		for(int i = 1; i <= 4; i++)
		{
			Coord next = cur + Coord{dx[i], dy[i]};
			if(in_zone(next) && snake_map[next.x][next.y] == 0)
			{
				q.push(next);
				snake_map[next.x][next.y] = 2;
			}
		}
	}
	int ret = 0;
	for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < 16; j++)
		{
			if(snake_map[i][j] != 2)
				ret++;
		}
	}
	return ret;
}


void game_over( int gameover_type, int winner, int p0_score, int p1_score )
{
	fprintf( stderr, "%d %d %d %d", gameover_type, winner, p0_score, p1_score );
}

int main( int argc, char** argv ) { SnakeGoAI start( argc, argv ); }