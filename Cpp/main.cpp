#include "adk.hpp"
#include <cmath>
#include <iostream>

const double PAR_SNAKE_LENGTH = 1.0;
const double PAR_LENGTH_BANK = 0.95;
const double PAR_SNAKE_COUNT[] = {0, 1, 1.15, 1.3, 1.27};
const double PAR_WALL_POTENTIALY_DESTROY = 0.25;

const double MAP_SIZE = 16;

const int MAX_SEARCH_LEVEL = 7;
Operation OP[7] = {OP_RIGHT, OP_RIGHT, OP_UP, OP_LEFT, OP_DOWN, OP_RAILGUN, OP_SPLIT};
double search(int level, Context &ctx, int player)
{
	if(level == MAX_SEARCH_LEVEL)
	{
		//return evaluate(ctx);
		return evaluate2(ctx, player);
	}
	int op_snake;
	bool valid_move;

	Snake snake_to_operate = ctx.current_snake();
	//ok here
	double ret = (player == ctx.current_player()) ? -1e9 : 1e9;
	/*
	for(int i = 1; i <= 6; i++)
	{
		Context ctx_tmp = ctx;
		valid_move = ctx_tmp.do_operation(OP[i]);
		if(valid_move)
		{
			if(player == ctx.current_player())
				ret = std::max(ret,search(level + 1, ctx_tmp, player));
			else
				ret = std::min(ret,search(level + 1, ctx_tmp, player));
		}
	}
	*/
	static int dx[5] = { -1, 1, 0, -1, 0 };
	static int dy[5] = { -1, 0, 1, 0, -1 };

	for(int i = 1; i <= 4; i++)
	{
		
	//确定移动方向是否合法
	
		//蛇头到达位置
		int t_x = snake_to_operate[0].x + dx[i];
		int t_y = snake_to_operate[0].y + dy[i];

		//超出边界
		if ( t_x >= ctx.length() || t_x < 0 || t_y >= ctx.width() || t_y < 0 )
		{
			//move[dir] = -1;
			continue;
		}

		//撞墙
		if ( ctx.wall_map()[t_x][t_y] != -1 )
		{
			//move[dir] = -1;
			continue;
		}

		//撞非本蛇
		if ( ctx.snake_map()[t_x][t_y] != -1 && ctx.snake_map()[t_x][t_y] != snake_to_operate.id )
		{
			//move[dir] = -1;
			continue;
		}

		//回头撞自己
		if ( ( snake_to_operate.length() > 2 ||
			   ( snake_to_operate.length() == 2 && snake_to_operate.length_bank > 0 ) ) &&
			 snake_to_operate[1].x == t_x && snake_to_operate[1].y == t_y )
		{
			//move[dir] = -1;
			continue;
		}
		Context ctx_tmp = ctx;
		//ok
		if(ctx_tmp.do_operation(OP[i]))
		{
			double score = search(level + 1, ctx_tmp, player);
			//ok
			if(player == ctx.current_player())
				ret = std::max(ret,score);
			else
				ret = std::min(ret,score);
		}
	}

	if(snake_to_operate.length() > 1)
	{
		if ( snake_to_operate.railgun_item.id != -1 )
		{
			Context ctx_tmp = ctx;
			if(ctx_tmp.do_operation(OP[5]))
			{
				double score = search(level + 1, ctx_tmp, player);
				if(player == ctx.current_player())
					ret = std::max(ret,score);
				else
					ret = std::min(ret,score);
			}
		}
		if ( ctx.my_snakes().size() < 4 )
		{
			Context ctx_tmp1 = ctx;
			if(ctx_tmp1.do_operation(OP[6]))
			{
				double score = search(level + 1, ctx_tmp1, player);
				if(player == ctx.current_player())
					ret = std::max(ret,score);
				else
					ret = std::min(ret,score);
			}
		}
	}
	return ret;
}

double evaluate2(const Context &ctx, int player)
{
	double my_score = 0;
	double enemy_score = 0;

	//蛇的长度&数量
	//长度银行
	//已固化的砖块


	for(int i = 0, len = ctx.my_snakes().size(); i < len; i++)
	{
		my_score += ctx.my_snakes()[i].coord_list.size() * PAR_SNAKE_LENGTH;
		my_score += ctx.my_snakes()[i].length_bank * PAR_LENGTH_BANK;
	}
	for(int i = 0, len = ctx.opponents_snakes().size(); i < len; i++)
	{
		enemy_score += ctx.opponents_snakes()[i].coord_list.size() * PAR_SNAKE_LENGTH;
		my_score += ctx.opponents_snakes()[i].length_bank * PAR_LENGTH_BANK;
	}

	my_score *= PAR_SNAKE_COUNT[ctx.my_snakes().size()];
	enemy_score *= PAR_SNAKE_COUNT[ctx.opponents_snakes().size()];

	for(int i = 0; i < MAP_SIZE; i++)
	{
		for(int j = 0; j < MAP_SIZE; j++)
		{
			if(ctx.wall_map()[i][j] == player)
				my_score++;
			if(ctx.wall_map()[i][j] == (player^1))
				enemy_score++;
		}
	}

	for(int i = 0; i < ctx.my_snakes().size(); i++)
	{
		int min_dist = 50;
		for(int j = 0; j < ctx.item_list().size(); j++)
		{
			if ( ctx.item_list()[j].time + ITEM_EXPIRE_LIMIT < ctx.current_round() )
				continue;
			int distance = abs( ctx.item_list()[j].x - ctx.my_snakes()[i].coord_list[0].x ) +
				abs( ctx.item_list()[j].y - ctx.my_snakes()[i].coord_list[0].y );
			if ( ctx.item_list()[j].time <= ctx.current_round() + distance &&
				 ctx.item_list()[j].time + 16 > ctx.current_round() + distance )
				min_dist = std::min(min_dist, distance);
		}
		my_score += 0.05 * (50 - min_dist);
	}
	for(int i = 0; i < ctx.opponents_snakes().size(); i++)
	{
		int min_dist = 50;
		for(int j = 0; j < ctx.item_list().size(); j++)
		{
			if ( ctx.item_list()[j].time + ITEM_EXPIRE_LIMIT < ctx.current_round() )
				continue;
			int distance = abs( ctx.item_list()[j].x - ctx.opponents_snakes()[i].coord_list[0].x ) +
				abs( ctx.item_list()[j].y - ctx.opponents_snakes()[i].coord_list[0].y );
			if ( ctx.item_list()[j].time <= ctx.current_round() + distance &&
				 ctx.item_list()[j].time + 16 > ctx.current_round() + distance )
				min_dist = std::min(min_dist, distance);
		}
		enemy_score += 0.05 * (50 - min_dist);
	}

	if(ctx.my_snakes().size() == 0)
		my_score = 0;
	if(ctx.opponents_snakes().size() == 0)
		enemy_score = 0;

	return my_score - enemy_score;
}

double evaluate(const Context &ctx)
{
	double my_score = 0;
	double enemy_score = 0;

	//蛇的长度&数量
	//长度银行
	//已固化的砖块
	//道具


	for(int i = 0, len = ctx.my_snakes().size(); i < len; i++)
	{
		my_score += ctx.my_snakes()[i].coord_list.size() * PAR_SNAKE_LENGTH;
		my_score += ctx.my_snakes()[i].length_bank * PAR_LENGTH_BANK;
	}
	for(int i = 0, len = ctx.opponents_snakes().size(); i < len; i++)
	{
		enemy_score += ctx.opponents_snakes()[i].coord_list.size() * PAR_SNAKE_LENGTH;
		my_score += ctx.opponents_snakes()[i].length_bank * PAR_LENGTH_BANK;
	}
	my_score *= PAR_SNAKE_COUNT[ctx.my_snakes().size()];
	enemy_score *= PAR_SNAKE_COUNT[ctx.opponents_snakes().size()];

	int my_wall = 0;
	int my_wall_line = 0;
	int enemy_wall = 0;
	int enemy_wall_line = 0;
	bool has_my_wall = false;
	bool has_enemy_wall = false;

	for(int i = 0; i < MAP_SIZE; i++)
	{
		has_my_wall = has_enemy_wall = false;
		for(int j = 0; j < MAP_SIZE; j++)
		{
			if(ctx.wall_map()[i][j] == ctx.current_player())
				my_wall++, has_my_wall = true;
			if(ctx.wall_map()[i][j] == (ctx.current_player()^1))
				enemy_wall++, has_enemy_wall = true;	
		}
		my_wall_line += has_my_wall;
		enemy_wall_line += has_enemy_wall;
	}

	// TODO: 区分敌人手里的融化射线和地图上的融化射线的数量
	my_wall *= (my_wall_line-1) * (1-PAR_WALL_POTENTIALY_DESTROY) / my_wall_line;
	enemy_wall *= (enemy_wall_line-1) * (1-PAR_WALL_POTENTIALY_DESTROY) / enemy_wall_line;
	my_score += my_wall;
	enemy_score += enemy_wall;

	return my_score - enemy_score;
}

Operation make_your_decision( const Snake& snake_to_operate, const Context& ctx, const OpHistory& op_list )
{
	//ok
	//Context ctx_tmp = ctx;
	//ok
	int maxid = 1;
	double maxscore = -1e9;

	static int dx[5] = { -1, 1, 0, -1, 0 };
	static int dy[5] = { -1, 0, 1, 0, -1 };

	for(int i = 1; i <= 4; i++)
	{
		
	//确定移动方向是否合法

		//蛇头到达位置
		int t_x = snake_to_operate[0].x + dx[i];
		int t_y = snake_to_operate[0].y + dy[i];

		//回头撞自己
		if ( ( snake_to_operate.length() > 2 ||
			   ( snake_to_operate.length() == 2 && snake_to_operate.length_bank > 0 ) ) &&
			 snake_to_operate[1].x == t_x && snake_to_operate[1].y == t_y )
		{
			//move[dir] = -1;
			if(maxid == i)
				maxid++;
			continue;
		}

		//超出边界
		if ( t_x >= ctx.length() || t_x < 0 || t_y >= ctx.width() || t_y < 0 )
		{
			//move[dir] = -1;
			continue;
		}

		//撞墙
		if ( ctx.wall_map()[t_x][t_y] != -1 )
		{
			//move[dir] = -1;
			continue;
		}

		//撞非本蛇
		if ( ctx.snake_map()[t_x][t_y] != -1 && ctx.snake_map()[t_x][t_y] != snake_to_operate.id )
		{
			//move[dir] = -1;
			continue;
		}

		
		
		//ok
		//ctx_tmp = ctx;
		Context ctx_tmp = ctx;
		//tle(disconnect)
		int testx = ctx.my_snakes()[0].coord_list.size();
		if(ctx_tmp.do_operation(OP[i]))
		{
			if(ctx.my_snakes()[0].coord_list[0].x == ctx_tmp.my_snakes()[0].coord_list[0].x
			&& ctx.my_snakes()[0].coord_list[0].y == ctx_tmp.my_snakes()[0].coord_list[0].y
			&& testx == ctx_tmp.my_snakes()[0].coord_list.size())
			{
				return OP[5];
			}
			double score = search(0, ctx_tmp, ctx.current_player());
			if(score > maxscore)
			{
				maxscore = score;
				maxid = i;
			}
		}
	}
	if(snake_to_operate.length() > 1)
	{
		if ( snake_to_operate.railgun_item.id != -1 )
		{
			Context ctx_tmp = ctx;
			if(ctx_tmp.do_operation(OP[5]))
			{
				double score = search(0, ctx_tmp, ctx.current_player());
				if(score > maxscore)
				{
					maxscore = score;
					maxid = 5;
				}
			}
		}
		if ( ctx.my_snakes().size() < 4 )
		{
			Context ctx_tmp1 = ctx;
			if(ctx_tmp1.do_operation(OP[6]))
			{
				double score = search(0, ctx_tmp1, ctx.current_player());
				if(score > maxscore)
				{
					maxscore = score;
					maxid = 6;
				}
			}
		}
	}
	return OP[maxid];
	/*


	//若该蛇有道具，执行融化射线操作
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
	*/
}

void game_over( int gameover_type, int winner, int p0_score, int p1_score )
{
	fprintf( stderr, "%d %d %d %d", gameover_type, winner, p0_score, p1_score );
}

int main( int argc, char** argv ) { SnakeGoAI start( argc, argv ); }