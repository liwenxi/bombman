/*
   注意！！！
   先修改服务器地址的端口号在运行
   recive中的的connect_to_server
 */
#include	<curses.h>
#include	<stdlib.h>
#include	<pthread.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<time.h>
#include	<strings.h>
#include	<string.h>

//存储炸弹的队列
typedef struct QNode{                                                   //队列的结构体声明
	int x;
	int y;
	struct QNode *next;
}QNode, *QueuePtr;

typedef struct{                                                         //队列的结构体声明
	QueuePtr front;
	QueuePtr rear;
}LinkQueue;

typedef struct{
	int x;
	int y;
}coordinate;

/*初始化类*/
void initial();
void level1();

/*多线程类*/
void *walk();
void *popo();
void *recive();

/*功能类*/
int connect_to_server(char *host, int portnum);
void *popopo(void *coor);
void put_wall(int x, int y);
void put_tree(int x, int y);
void put_block(int x, int y);
void destory_square(int x, int y);
void queue_waiting(int i);
void queue_signal(int i);
int init_queue();
int queue_is_empty();
int enter_queue(int x, int y);
int dele_queue(int *x, int *y);
void talk_with_server(int fd);
void init_player_table();
void init_p_table();
void find_player(int *x, int *y, int num);
void death(char num);
void win();
void bye();

/*全局变量*/
LinkQueue wait_queue;                           //炸弹队列

int player_table[12][15];                       //玩家位置图

int p_table[12][15];                            //炸弹位置图

int now_fd;                                     //当前的连接

int queue_lock = 0;                             //炸弹队列互斥锁

int x;                                          //玩家的坐标
int y;


int wall_now[12][15];                           //当前的墙的状态
int tree_now[12][15];                           //当前的树的状态
char title[100];                                //游戏的标题

int num_in_server;                              //与服务器建立的连接

int main()
{
	pthread_t thread[10];

	pthread_create(&thread[2], NULL, recive, NULL); //首先建立连接，连接之后再进行页面的创建等活动
	sleep(3);
	initial();
	level1();
	init_queue();
	init_player_table();
	init_p_table();
	pthread_create(&thread[0], NULL, walk, NULL);   //走步线程
	pthread_create(&thread[1], NULL, popo, NULL);   //炸掉检测线程
	pthread_join(thread[0], NULL);                  //等待退出
}


/**** 初始化类函数 ***/

//对curses进行初始化设置
void initial()
{
	initscr();                                      //开启curses模式
	cbreak();                                       //开启cbreak模式,除了 DELETE 或 CTRL 等
	//仍被视为特殊控制字元外一切输入的字元将立刻被一一读取
	nonl();                                         //用来决定当输入资料时, 按下 RETURN 键是否被对应为 NEWLINE 字元
	noecho();                                       //echo() and noecho(): 此函式用来控制从键盘输入字元时是否将字元显示在终端机上
	intrflush(stdscr,false);
	keypad(stdscr,true);                            //当开启 keypad 后, 可以使用键盘上的一些特殊字元, 如上下左右>等方向键
	refresh();                                      //将做清除荧幕的工作
}

//对当前地图进行初始化设置
void level1(){
	int i, j;
	char name[BUFSIZ];
	int tree[12][15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	int wall[12][15] = {0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0,
		0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0,
		0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0,
		0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0,
		0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0,
		0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0,
		0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0};
	strcpy(name, "pla");
	name[3] = '0' + num_in_server;
	name[4] = '\0';
	for (i = 0; i < 12; i++){
		for (j = 0; j < 15; j++){
			tree_now[i][j] = tree[i][j];
			wall_now[i][j] = wall[i][j];
		}
	}
	//绘制主体框架
	mvwvline(stdscr, 1, 0, '|', 24);
	mvwhline(stdscr, 0, 0, '-', 73);
	mvwhline(stdscr, 25, 0, '-', 73);
	mvwvline(stdscr, 1, 61, '|', 24);
	mvwvline(stdscr, 1, 72, '|', 24);
	attron(A_REVERSE);                              //开启反白模式
	sprintf(title,"Let's pop!;windows[%d,%d]", COLS, LINES);
	mvaddstr(0,20,title);                           //移动到(0,20)位置打印title
	mvaddstr(2,63,"POPER MAN");
	attroff(A_REVERSE);                             //关闭反白模式
	for (i = 0; i < 12; i++){
		for (j = 0; j < 15; j++){
			if (tree[i][j] == 1)
				put_tree((j+1)*4-3, (i+1)*2-1);
		}
	}
	for (i = 0; i < 12; i++){
		for (j = 0; j < 15; j++){
			if (wall[i][j] == 1)
				put_wall((j+1)*4-3, (i+1)*2-1);
		}
	}
	//判断初始位置
	if (num_in_server == 0){
		x = 1;
		y = 1;
	}else if (num_in_server == 1){
		x = 57;
		y = 1;
	}else if (num_in_server == 2){
		x = 1;
		y = 23;
	}else if (num_in_server == 3){
		x = 57;
		y = 23;	
	}else{
		x = 1;
		y = 1;
	}
	mvaddstr(y, x, name);
	move(y,x);                                      //移动坐标到(1,1)
}

//初始化炸点列表
void init_p_table(){
	int i, j;
	int p[12][15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	for (i = 0; i < 12; i++){
		for (j = 0; j < 15; j++){
			p_table[i][j] =  p[i][j];
		}
	}
}

//初始化玩家的位置列表
void init_player_table(){
	int i, j;
	int player[12][15] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4};
	for (i = 0; i < 12; i++){
		for (j = 0; j < 15; j++){
			player_table[i][j] =  player[i][j];
		}
	}
}

//初始化队列
int init_queue(){
	wait_queue.front = wait_queue.rear = (QueuePtr)malloc(sizeof(QNode));
	wait_queue.front->next = NULL;
	return 1;
}

/*************** 多线程类函数 ****************/
//走步及炸弹放置函数
void *walk(){
	coordinate coor;
	char buf[BUFSIZ];
	char name[BUFSIZ];
	char prechar[10];
	int x_old, y_old;
	int i, j;
	strcpy(name, "pla");
	name[3] = '0' + num_in_server;
	const int WIDTH = COLS-2;
	const int HIGHT = LINES-2;
	int ch;
	int temp;
	find_player(&x_old, &y_old, name[3] - '0' + 1);
	for (i = 0; i < 12; i++){
		for (j = 0; j < 15; j++){
			if (player_table[i][j] == name[3] - '0' + 1){
				player_table[i][j] = 0;
			}
		}
	}
	do{
		ch = getch();//从键盘读取一个字元. (注意! 传回的是整数值)
		mvaddstr(y,x,"    ");
		//注意判断移动过程中的障碍物
		switch(ch)
		{
			case KEY_UP:
				--y;
				--y;
				if (player_table[(y+1)/2-1][(x+3)/4-1] != 0 || p_table[(y+1)/2-1][(x+3)/4-1] != 0 || tree_now[(y+1)/2-1][(x+3)/4-1] == 1 || wall_now[(y+1)/2-1][(x+3)/4-1] == 1)
					y += 2;
				break;
			case KEY_DOWN:
				++y;
				++y;
				if (player_table[(y+1)/2-1][(x+3)/4-1] != 0 || p_table[(y+1)/2-1][(x+3)/4-1] != 0 || tree_now[(y+1)/2-1][(x+3)/4-1] == 1 || wall_now[(y+1)/2-1][(x+3)/4-1] == 1)
					y -= 2;
				break;
			case KEY_RIGHT:
				x+=4;
				if (player_table[(y+1)/2-1][(x+3)/4-1] != 0 || p_table[(y+1)/2-1][(x+3)/4-1] != 0 || tree_now[(y+1)/2-1][(x+3)/4-1] == 1 || wall_now[(y+1)/2-1][(x+3)/4-1] == 1)
					x -= 4;
				break;
			case KEY_LEFT:
				x-=4;
				if (player_table[(y+1)/2-1][(x+3)/4-1] != 0 || p_table[(y+1)/2-1][(x+3)/4-1] != 0 || tree_now[(y+1)/2-1][(x+3)/4-1] == 1 || wall_now[(y+1)/2-1][(x+3)/4-1] == 1)
					x +=4;
				break;
			case ' ':
				mvaddch(y+1,x,'P');
				strcpy(buf, "PUTP");
				buf[4] = '0' + x / 10;
				buf[5] = '0' + x % 10;
				buf[6] = '0' + y / 10;
				buf[7] = '0' + y %10;
				write(now_fd, buf, 10);
				strcpy(buf, "          ");
				//		popopo((void * )&coor);
				break;
			case '\t':
				x+=7;
				break;
			case KEY_BACKSPACE:
				mvaddch(y+1,x,'P');
				//              coor.x = x;
				//              coor.y = y;
				strcpy(buf, "PUTP");
				buf[4] = '0' + x / 10;
				buf[5] = '0' + x % 10;
				buf[6] = '0' + y / 10;
				buf[7] = '0' + y %10;
				write(now_fd, buf, 10);
				strcpy(buf, "          ");
				//              popopo((void * )&coor);
				break;
			case 13:
				++y;
				x = 1;
				break;
			case 27:
				strcpy(buf, "DEAD");
				write(now_fd, buf, 10);
				sleep(1);
				bye();
				endwin(); //关闭curses 模式, 或是暂时的跳离 curses 模式
				sleep(1);
				exit(1);
			default:
				//	addch(ch);//显示当前输入的字元
				//	x++;
				break;
		}
		//注意不能越过边框
		if (x <= 0)
			x = 1;
		if (x >= 61)
			x = 57;
		if (y <= 0)
			y = 1;
		if (y >= 25)
			y = 23;
		sprintf(title,"[%d,%d]", x, y);
		mvaddstr(0,0,title);
		mvaddstr(y,x,name);
		move(y, x);
		strcpy(buf, "MOVE");
		buf[4] = '0' + x / 10;
		buf[5] = '0' + x % 10;
		buf[6] = '0' + y / 10;
		buf[7] = '0' + y %10;
		buf[8] = '0' + num_in_server;
		write(now_fd, buf, 10);
	}while(1);
	return NULL;
}
//接收服务器的消息，会调用talk_with_server，其中包含一个循环
void *recive(){
	int fd;
	now_fd = fd=connect_to_server("MacBook-Pro-84.local", 9000);	/*call the server*/
	if(fd==-1){
		printf("no server!\n");
		exit(1);	 				/*or die*/
	}
	talk_with_server(fd);			/*chat with server*/
	printf("ok!\n");
	close(fd);						/*hang up when done*/
}
//炸弹函数，会不断从队列中弹出炸弹
void *popo(){
	int x = 1;
	int y = 1;
	coordinate coor;
	int count = 0;
	sleep(1);
	while(1){
		if(dele_queue(&x, &y)){
			queue_waiting(queue_lock);
			coor.x = x;
			coor.y = y;
			popopo((void *)&coor);
			queue_signal(queue_lock);
			count++;
		}
	}
}


/*************** 功能类函数 *****************/

//在炸弹范围内销毁可以去掉的墙壁，传入大坐标
void destory_square(int x, int y){
	if ((wall_now[(y+1)/2-1][(x+3+4)/4-1] == 1) && (((y+1)/2-1) >= 0 && ((y+1)/2-1) < 12) && (((x+3+4)/4-1)>=0 && ((x+3+4)/4-1) < 15)){
		wall_now[(y+1)/2-1][(x+3+4)/4-1] = 0;
		put_block(x+4, y);
	}
	if ((wall_now[(y+1)/2-1][(x+3-4)/4-1] == 1)  && (((y+1)/2-1) >= 0 && ((y+1)/2-1) < 12) && (((x+3-4)/4-1)>=0 && ((x+3-4)/4-1) < 15)){
		wall_now[(y+1)/2-1][(x+3-4)/4-1] = 0;
		put_block(x-4, y);
	}
	if ((wall_now[(y+1+2)/2-1][(x+3)/4-1] == 1) && (((y+1+2)/2-1) >= 0 && ((y+1+2)/2-1) < 12) && (((x+3)/4-1)>=0 && ((x+3)/4-1) < 15)){
		wall_now[(y+1+2)/2-1][(x+3)/4-1] = 0;
		put_block(x, y+2);
	}
	if ((wall_now[(y+1-2)/2-1][(x+3)/4-1] == 1) && (((y+1-2)/2-1) >= 0 &&((y+1-2)/2-1) < 12) && (((x+3)/4-1)>=0 &&((x+3)/4-1) < 15)){
		wall_now[(y+1-2)/2-1][(x+3)/4-1] = 0;
		put_block(x, y-2);
	}
}
//放置棵树，不可被炸毁，传入大坐标
void put_tree(int x, int y){
	attron(A_REVERSE);
	mvaddstr(y,x,"XXXX");
	mvaddstr(y+1,x,"XXXX");
	attroff(A_REVERSE);
}
//放置一面墙，可以被炸毁，传入大坐标
void put_wall(int x, int y){
	mvaddstr(y,x,"XXXX");
	mvaddstr(y+1,x,"XXXX");
}
//覆盖原有的屏幕区域，传入大坐标
void put_block(int x, int y){
	mvaddstr(y,x,"    ");
	mvaddstr(y+1,x,"    ");
}

//和服务器进行socket连接
int connect_to_server(char *host, int portnum)
{
	int sock;
	struct sockaddr_in  servadd;        /* the number to call */
	struct hostent      *hp;            /* used to get number */

	/** Step 1: Get a socket **/

	sock = socket( AF_INET, SOCK_STREAM, 0 );    /* get a line   */
	if ( sock == -1 )
		return -1;

	/** Step 2: connect to server **/

	bzero( &servadd, sizeof(servadd) );     /* zero the address     */
	hp = gethostbyname( host );             /* lookup host's ip #   */
	if (hp == NULL)
		return -1;
	bcopy(hp->h_addr, (struct sockaddr *)&servadd.sin_addr, hp->h_length);
	servadd.sin_port = htons(portnum);      /* fill in port number  */
	servadd.sin_family = AF_INET ;          /* fill in socket type  */

	if ( connect(sock,(struct sockaddr *)&servadd, sizeof(servadd)) !=0)
		return -1;

	return sock;
}
//解析服务器的消息
void talk_with_server(int fd){
	char buf[256];
	char name[5];
	int x_in, y_in;
	int x_old, y_old;
	int i, j;
	strcpy(name, "pla");
	name[4] = '\0';
	while(1){
		read(fd, buf, 10);
		if (strncmp(buf, "PO", 2) == 0){                    //服务器发来爆炸指令
			//			printf("%s\n", buf);
			x_in = (buf[4]-'0')*10 + (buf[5]-'0');
			y_in = (buf[6]-'0')*10 + (buf[7]-'0');
			enter_queue(x_in, y_in);
			strcpy(buf, "          ");
		}
		if (strncmp(buf, "NUM", 3) == 0){                   //服务器通知客户端上线的编号
			printf("%s\n", buf);
			num_in_server = buf[3]-'0';
			strcpy(buf, "          ");
		}
		if (strncmp(buf, "PUTP", 4) == 0){                  //放置炸弹
			//                      printf("%s\n", buf);
			x_in = (buf[4]-'0')*10 + (buf[5]-'0');
			y_in = (buf[6]-'0')*10 + (buf[7]-'0');
			mvaddch(y_in+1,x_in,'P');
			p_table[(y_in+1)/2-1][(x_in+3)/4-1] = 1;
			refresh();
			move(y, x);
			strcpy(buf, "          ");
		}
		if (strncmp(buf, "MOVE", 4) == 0){                  //移动
			//                      printf("%s\n", buf);
			name[3] = buf[8];
			x_in = (buf[4]-'0')*10 + (buf[5]-'0');
			y_in = (buf[6]-'0')*10 + (buf[7]-'0');
			find_player(&x_old, &y_old, name[3] - '0' + 1);
			for (i = 0; i < 12; i++){
				for (j = 0; j < 15; j++){
					if (player_table[i][j] == name[3] - '0' + 1){
						player_table[i][j] = 0;
					}
				}
			}
			mvaddstr(y_old, x_old, "    ");
			player_table[(y_in+1)/2-1][(x_in+3)/4-1] = buf[8] - '0' + 1;
			mvaddstr(y_in, x_in, name);
			refresh();
			//			move(y, x);
			strcpy(buf, "          ");
		}
		if (strncmp(buf, "DEAD", 4) == 0){                  //死亡信息判断，是自己就结束游戏，否则清楚另一玩家
			if (buf[8] == '0' + num_in_server){
				write(now_fd, buf, 10);
				sleep(1);
				death(buf[9]);
				endwin(); //关闭curses 模式, 或是暂时的跳离 curses 模式
				sleep(1);
				exit(1);
			}
			find_player(&x_old, &y_old, buf[8] - '0' + 1);
			for (i = 0; i < 12; i++){
				for (j = 0; j < 15; j++){
					if (player_table[i][j] == buf[8] - '0' + 1){
						player_table[i][j] = 0;
					}
				}
			}
			mvaddstr(y_old, x_old, "    ");
			refresh();

			move(y, x);
			if (buf[9]- '0' == 2){
				write(now_fd, buf, 10);
				sleep(1);
				win();
				endwin();
				sleep(1);
				exit(0);
			}
			strcpy(buf, "          ");
		}
	}
}
//爆炸函数，其中爆炸效果是使用新创建的两个pad
void *popopo(void *coor){
	WINDOW *alertWindow;
	WINDOW *alertWindow1;
	int i;
	int j;
	coordinate *temp = (coordinate *)coor;
	i = temp->x;
	j = temp->y;
	attron(A_REVERSE);
	alertWindow = newpad(2, 12);
	mvwprintw(alertWindow, 0, 0, "%s", "OOOOOOOOOOOO");
	mvwprintw(alertWindow, 1, 0, "%s", "OOOOOOOOOOOO");
	prefresh(alertWindow, 0, 0, j, i-4, j+2, i+8);
	alertWindow1 = newpad(6, 4);
	mvwprintw(alertWindow1, 0, 0, "%s", "OOOO");
	mvwprintw(alertWindow1, 1, 0, "%s", "OOOO");
	mvwprintw(alertWindow1, 2, 0, "%s", "OOOO");
	mvwprintw(alertWindow1, 3, 0, "%s", "OOOO");
	mvwprintw(alertWindow1, 4, 0, "%s", "OOOO");
	mvwprintw(alertWindow1, 5, 0, "%s", "OOOO");
	prefresh(alertWindow1, 0, 0, j-2, i, j+4, i+4);
	usleep(300000);
	//	usleep(100000);
	attroff(A_REVERSE);
	touchwin(stdscr);
	wrefresh(stdscr);
	delwin(alertWindow);
	delwin(alertWindow1);
	mvaddstr(j+1,i," ");
	destory_square(i, j);
	refresh();
	p_table[(j+1)/2-1][(i+3)/4-1] = 0;
	move(x, y);
}
//死亡时候的提示窗口
void death(char num){
	char temp[2];
	WINDOW *alertWindow;
	alertWindow = newwin(8, 30, 6, 20);
	box(alertWindow, '|', '-');
	temp[0] = num;
	temp[1] = '\0';
	mvwaddstr(alertWindow, 2, 8, "Game Over!");
	mvwaddstr(alertWindow, 4, 8, "rank:");
	mvwaddstr(alertWindow, 4, 14, temp);
	wattron(alertWindow, A_REVERSE);
	mvwaddstr(alertWindow, 6, 13, "OK");
	wattroff(alertWindow, A_REVERSE);
	touchwin(alertWindow);
	wrefresh(alertWindow);
	getch();
	touchwin(stdscr);
	wrefresh(stdscr);
	delwin(alertWindow);
	move(x, y);
}
//获胜时候的提示窗口
void win(){
	WINDOW *alertWindow;
	alertWindow = newwin(8, 30, 6, 20);
	box(alertWindow, '|', '-');
	mvwaddstr(alertWindow, 2, 8, "You are the winner!");
	mvwaddstr(alertWindow, 4, 8, "rank: 1");
	wattron(alertWindow, A_REVERSE);
	mvwaddstr(alertWindow, 6, 13, "OK");
	wattroff(alertWindow, A_REVERSE);
	touchwin(alertWindow);
	wrefresh(alertWindow);
	getch();
	touchwin(stdscr);
	wrefresh(stdscr);
	delwin(alertWindow);
	move(x, y);
}
//按esc退出时候的窗口
void bye(){
        WINDOW *alertWindow;
        alertWindow = newwin(8, 30, 6, 20);
        box(alertWindow, '|', '-');
        mvwaddstr(alertWindow, 2, 4, "Welcome to play again!");
        mvwaddstr(alertWindow, 4, 10, "BYE BYE");
        wattron(alertWindow, A_REVERSE);
        mvwaddstr(alertWindow, 6, 13, "OK");
        wattroff(alertWindow, A_REVERSE);
        touchwin(alertWindow);
        wrefresh(alertWindow);
        getch();
        touchwin(stdscr);
        wrefresh(stdscr);
        delwin(alertWindow);
        move(x, y);
}
//备份，爆炸效果，pad制作
void popopo_pad_copy(int x, int y){
	WINDOW *alertWindow;
	WINDOW *alertWindow1;
	alertWindow = newpad(2, 12);
	mvwprintw(alertWindow, 0, 0, "%s", "OOOOOOOOOOOO");
	mvwprintw(alertWindow, 1, 0, "%s", "OOOOOOOOOOOO");
	prefresh(alertWindow, 0, 0, y, x-4, y+2, x+8);
	alertWindow1 = newpad(6, 4);
	mvwprintw(alertWindow1, 0, 0, "%s", "OOOO");
	mvwprintw(alertWindow1, 1, 0, "%s", "OOOO");
	mvwprintw(alertWindow1, 2, 0, "%s", "OOOO");
	mvwprintw(alertWindow1, 3, 0, "%s", "OOOO");
	mvwprintw(alertWindow1, 4, 0, "%s", "OOOO");
	mvwprintw(alertWindow1, 5, 0, "%s", "OOOO");
	prefresh(alertWindow1, 0, 0, y-2, x, y+4, x+4);
	usleep(300000);
	touchwin(stdscr);
	wrefresh(stdscr);
	delwin(alertWindow);
	delwin(alertWindow1);
	mvaddstr(y,x," ");
}
//备份，爆炸效果，win制作
void popopo_copy(int x, int y){
	WINDOW *alertWindow;
	WINDOW *alertWindow1;
	alertWindow = newwin(2, 12, y, x-4);
	//      box(alertWindow, '|', '-');
	mvwaddstr(alertWindow, 0, 0, "OOOOOOOOOOOO");
	mvwaddstr(alertWindow, 1, 0, "OOOOOOOOOOOO");
	touchwin(alertWindow);
	wrefresh(alertWindow);
	alertWindow1 = newwin(6, 4, y-2, x);
	mvwaddstr(alertWindow1, 0, 0, "OOOO");
	mvwaddstr(alertWindow1, 1, 0, "OOOO");
	mvwaddstr(alertWindow1, 2, 0, "OOOO");
	mvwaddstr(alertWindow1, 3, 0, "OOOO");
	mvwaddstr(alertWindow1, 4, 0, "OOOO");
	mvwaddstr(alertWindow1, 5, 0, "OOOO");
	touchwin(alertWindow1);
	wrefresh(alertWindow1);
	sleep(1);
	touchwin(stdscr);
	wrefresh(stdscr);
	delwin(alertWindow);
	delwin(alertWindow1);
}
//判断队列是否为空
int queue_is_empty(){
	if (wait_queue.rear == wait_queue.front){
		return 1;
	}else{
		return 0;
	}
}
//入队列操作，将炸弹进入爆炸队列，加锁
int enter_queue(int x, int y){
	QueuePtr p = (QueuePtr)malloc(sizeof(QNode));
	queue_waiting(queue_lock);
	p->x = x;
	p->y = y;
	p->next = NULL;
	wait_queue.rear->next = p;
	wait_queue.rear = p;
	queue_signal(queue_lock);
	return 1;
}
//出队列，将炸弹出队列处理，调用前后加锁，内部无锁
int dele_queue(int *x, int *y){
	if (wait_queue.rear == wait_queue.front){
		return 0;
	}
	QueuePtr p;
	//	queue_waiting(queue_lock);
	p = wait_queue.front->next;
	*x = p->x;
	*y = p->y;
	wait_queue.front->next = p->next;
	if (wait_queue.rear == p){
		wait_queue.rear= wait_queue.rear = wait_queue.front;
	}
	free(p);
	//	queue_signal(queue_lock);
	return 1;
}

//信号量函数waiting
void queue_waiting(int i){
	while(i);
	i++;
}
//信号量函数signal
void queue_signal(int i){
	i--;
}
//查找当前玩家列表中指定玩家的位置，返回准确地址
void find_player(int *x, int *y, int num){
	int i, j;
	for (i = 0; i < 12; i++){
		for (j = 0; j < 15; j++){
			if (player_table[i][j] == num){
				*x = (j+1)*4-3;
				*y = (i+1)*2-1;

			}
		}
	}
}
