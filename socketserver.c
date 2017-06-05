#include	<stdio.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<time.h>
#include	<strings.h>
#include	<stdlib.h>
#include	<pthread.h>
#define   HOSTLEN  256
#define	  BACKLOG  4


/******队列的声明******/
typedef struct QNode{                                                   //队列的结构体声明
	int x;
	int y;
	int time;
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
/********************/

/*初始化类*/
int init_queue();

/*线程类*/
void *process_request();
void *send_po();

/*功能类函数*/
int queue_is_empty();

int make_server_socket(int portnum);

int make_server_socket_q(int portnum, int backlog);

int enter_queue(int x, int y);

int dele_queue(int *x, int *y);

void queue_waiting(int i);

void queue_signal(int i);

int if_dele();

void init_player_table();

void find_player(int *x, int *y, int num);

void send_dead(int x, int y);

/*全局变量*/
LinkQueue wait_queue;                               //炸弹队列

int now_fd[10];                                     //存储所有的客户端连接
int count_fd = 0;                                   //判断有几个客户登陆
int lock_queue_po = 0;                              //信号量
int alive = 0;                                      //判断有几个活着的玩家
int player_table[12][15];



int main(){
	int sock, fd;                           /*socket and connection*/
	pthread_t thread[10];
	char buf[BUFSIZ];
	sock = make_server_socket(8080);
	if(sock==-1)
		exit(1);
	printf("%d\n", sock);
	init_queue();
	init_player_table();
	pthread_create(&thread[0], NULL, send_po, NULL);
	while(1){
		fd=accept(sock, NULL, NULL);    /*take next call*/
		if(fd==-1)
			break;                  /*or die*/
		printf("fd:%d\n", fd);
		strcpy(buf, "NUM");
		buf[3] = '0' + count_fd;
		write(fd, buf, 10);
		printf("%s\n", buf);
		now_fd[count_fd++] = fd;
		alive++;
		pthread_create(&thread[count_fd], NULL, process_request, NULL);
		//		process_request(fd);            /*chat with client*/
		//		close(fd);                      /*hang up when done*/
	}
	return 0;
}

/*****初始化部分*****/

int init_queue(){
	wait_queue.front = wait_queue.rear = (QueuePtr)malloc(sizeof(QNode));
	wait_queue.front->next = NULL;
	return 1;
}

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
	printf("after init %d\n", player_table[0][0]);
}


/****** 多线程部分 *********/
//处理每个客户端发过来的消息
void *process_request()
{
    int fd = now_fd[count_fd-1];
    char	cmd[BUFSIZ], arg[BUFSIZ], buf[BUFSIZ];
    char name[BUFSIZ];
    int x, y;
    int x_old, y_old;
    int i, j;
    int temp;                                         //存储线程对应客户端的连接
    temp = count_fd-1;
    while(1){
        read(fd, buf, 10);
        printf("count:%d\n",count_fd);
        printf("read:%s\n", buf);
        if(fd==-1)
            break;
        if (strncmp(buf, "PUTP", 4) == 0){
            printf("PUTP:%s\n", buf);
            x = (buf[4]-'0')*10 + (buf[5]-'0');
            y = (buf[6]-'0')*10 + (buf[7]-'0');
            enter_queue(x, y);
            for (i = 0; i < count_fd; i++){
                if (now_fd[i] != -1)
                    write(now_fd[i], buf, 10);
            }
            printf("yifasong%s\n", buf);
            strcpy(buf, "          ");
        }
        if (strncmp(buf, "MOVE", 4) == 0){
            printf("MOVE:%s\n", buf);
            for (i = 0; i < count_fd; i++){
                if (i == (buf[8]-'0'))
                    continue;
                if (now_fd[i] != -1)
                    write(now_fd[i], buf, 10);
            }
            x = (buf[4]-'0')*10 + (buf[5]-'0');
            y = (buf[6]-'0')*10 + (buf[7]-'0');
            find_player(&x_old, &y_old, name[3] - '0' + 1);
            for (i = 0; i < 12; i++){
                for (j = 0; j < 15; j++){
                    if (player_table[i][j] == buf[8] - '0' + 1){
                        player_table[i][j] = 0;
                    }
                }
            }
            player_table[(y+1)/2-1][(x+3)/4-1] = buf[8] - '0' + 1;
            printf("already send %s\n", buf);
            strcpy(buf, "          ");
        }
        if (strncmp(buf, "DEAD", 4) == 0){
            now_fd[temp] = -1;
            break;
        }
    }
    printf("esc!\n");
}

//广播爆炸信息
void *send_po(){
    int x, y;
    int i;
    char buf[BUFSIZ];
    while(1){
        if (if_dele()){
            printf("ready to send\n");
            dele_queue(&x, &y);
            strcpy(buf, "PO  ");
            buf[4] = '0' + x / 10;
            buf[5] = '0' + x % 10;
            buf[6] = '0' + y / 10;
            buf[7] = '0' + y % 10;
            for (i = 0; i < count_fd; i++){
                write(now_fd[i], buf, 10);
                send_dead(x, y);
            }
            printf("already send%s\n", buf);
            strcpy(buf, "          ");
        }
    }
    printf("wrong in send_po\n");
}

/*******功能类函数***********/
int make_server_socket(int portnum)
{
    return make_server_socket_q(portnum, BACKLOG);
}
int make_server_socket_q(int portnum, int backlog)
{
    int i;
    struct  sockaddr_in   saddr;   /* build our address here */
    struct	hostent		*hp;   /* this is part of our    */
    char	hostname[HOSTLEN];     /* address 	         */
    int	sock_id;	       /* the socket             */
    int fd;
    sock_id = socket(PF_INET, SOCK_STREAM, 0);  /* get a socket */
    if ( sock_id == -1 )
        return -1;
    
    /** build address and bind it to socket **/
    
    bzero((void *)&saddr, sizeof(saddr));   /* clear out struct     */
    gethostname(hostname, HOSTLEN);         /* where am I ?         */
    hp = gethostbyname(hostname);           /* get info about host  */
    /* fill in host part    */
    bcopy( (void *)hp->h_addr, (void *)&saddr.sin_addr, hp->h_length);
    saddr.sin_port = htons(portnum);        /* fill in socket port  */
    saddr.sin_family = AF_INET ;            /* fill in addr family  */
    if ( bind(sock_id, (struct sockaddr *)&saddr, sizeof(saddr)) != 0 )
        return -1;
    
    /** arrange for incoming calls **/
    
    if ( listen(sock_id, backlog) != 0 )
        return -1;
    printf("server name: %s\n", hostname);
    return sock_id;
}

int queue_is_empty(){
    if (wait_queue.rear == wait_queue.front){
        return 1;
    }else{
        return 0;
    }
}
//入列，加锁
int enter_queue(int x, int y){
    QueuePtr p = (QueuePtr)malloc(sizeof(QNode));
    queue_waiting(lock_queue_po);
    p->x = x;
    p->y = y;
    p->time = (int)time( NULL );
    //	printf("%d\n", p->time);
    p->next = NULL;
    wait_queue.rear->next = p;
    wait_queue.rear = p;
    queue_signal(lock_queue_po);
    return 1;
}
//出列，调用前后加锁
int dele_queue(int *x, int *y){
    queue_waiting(lock_queue_po);
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
    queue_signal(lock_queue_po);
    return 1;
}
//判断队列中是否有即将爆炸的炸弹
int if_dele(){
    int temp;
    if (wait_queue.rear == wait_queue.front){
        //		printf("1111:%d\n", temp);
        return 0;
    }
    if ((temp = ((int)time( NULL )-wait_queue.front->next->time)) >= 2 ){
        return 1;
    }else{
        //		printf("22222:%d\n", temp);
        return 0;
    }
}

//信号量
void queue_waiting(int i){
    while(i);
    i++;
}

void queue_signal(int i){
    i--;
}

//发送死亡消息
void send_dead(int x, int y){
    /******************
     此处发送的死亡消息是全局坐标
     ********************/
    int i, temp;
    char buf[BUFSIZ];
    strcpy(buf, "DEAD");
    if ((player_table[(y+1)/2-1][(x+3+4)/4-1] != 0) && (((y+1)/2-1) >= 0 && ((y+1)/2-1) < 12) && (((x+3+4)/4-1)>=0 && ((x+3+4)/4-1) < 15)){
        temp = player_table[(y+1)/2-1][(x+3+4)/4-1];
        buf[4] = '0' + (x+4) / 10;
        buf[5] = '0' + (x+4) % 10;
        buf[6] = '0' + (y) / 10;
        buf[7] = '0' + (y) % 10;
        buf[8] = '0' + temp -1;
        buf[9] = '0' + alive--;
        for (i = 0; i < count_fd; i++){
            if (now_fd[i] != -1)
                write(now_fd[i], buf, 10);
            printf("DEAD:%s\n", buf);
        }
        player_table[(y+1)/2-1][(x+3+4)/4-1] = 0;
        //		put_block(x+4, y);
    }
    if ((player_table[(y+1)/2-1][(x+3-4)/4-1] != 0)  && (((y+1)/2-1) >= 0 && ((y+1)/2-1) < 12) && (((x+3-4)/4-1)>=0 && ((x+3-4)/4-1) < 15)){
        temp = player_table[(y+1)/2-1][(x+3-4)/4-1];
        buf[4] = '0' + (x-4) / 10;
        buf[5] = '0' + (x-4) % 10;
        buf[6] = '0' + (y) / 10;
        buf[7] = '0' + (y) % 10;
        buf[8] = '0' + temp - 1;
        buf[9] = '0' + alive--;
        for (i = 0; i < count_fd; i++){
            if (now_fd[i] != -1)
                write(now_fd[i], buf, 10);
            printf("DEAD:%s\n", buf);
        }
        player_table[(y+1)/2-1][(x+3-4)/4-1] = 0;
        //		put_block(x-4, y);
    }
    if ((player_table[(y+1+2)/2-1][(x+3)/4-1] != 0) && (((y+1+2)/2-1) >= 0 && ((y+1+2)/2-1) < 12) && (((x+3)/4-1)>=0 && ((x+3)/4-1) < 15)){
        temp = player_table[(y+1+2)/2-1][(x+3)/4-1];
        buf[4] = '0' + (x) / 10;
        buf[5] = '0' + (x) % 10;
        buf[6] = '0' + (y+2) / 10;
        buf[7] = '0' + (y+2) % 10;
        buf[8] = '0' + temp - 1;
        buf[9] = '0' + alive--;
        for (i = 0; i < count_fd; i++){
            if (now_fd[i] != -1)
                write(now_fd[i], buf, 10);
            printf("DEAD:%s\n", buf);
        }
        player_table[(y+1+2)/2-1][(x+3)/4-1] = 0;
        //		put_block(x, y+2);
    }
    if ((player_table[(y+1-2)/2-1][(x+3)/4-1] != 0) && (((y+1-2)/2-1) >= 0 &&((y+1-2)/2-1) < 12) && (((x+3)/4-1)>=0 &&((x+3)/4-1) < 15)){
        temp = player_table[(y+1-2)/2-1][(x+3)/4-1];
        buf[4] = '0' + (x) / 10;
        buf[5] = '0' + (x) % 10;
        buf[6] = '0' + (y-2) / 10;
        buf[7] = '0' + (y-2) % 10;
        buf[8] = '0' + temp - 1;
        buf[9] = '0' + alive--;
        for (i = 0; i < count_fd; i++){
            if (now_fd[i] != -1)
                write(now_fd[i], buf, 10);
            printf("DEAD:%s\n", buf);
        }
        player_table[(y+1-2)/2-1][(x+3)/4-1] = 0;
        //		put_block(x, y-2);
    }
    strcpy(buf, "          ");
}
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
