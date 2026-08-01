#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <termios.h>
#include <time.h>

#define BUF_SIZE 1024
#define MAX_CLNT 256

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

void error_handling(char *message);

typedef struct{
    int x, y;  // 좌표 값
    double clock;  // 현재 경과된 시간 
    int player_id;  // 각 플레이어의 번호 없을 시 0
}s_pkt;   

typedef struct{
    int player;  // 각 플레이어의 번호 없을 시 0
    int pillow;  // 0: no pillow, 1: Red, 2: Blue
}game_state;

typedef struct{
    int x, y;  // 좌표 값, x가 음수일 경우 enter키 입력 
    int pillow; // 0: default, 1: enter키 입력 
}c_pkt;

typedef struct{
    int player_num;  // 플레이어의 수 
    int size;  // 게임 사이즈
    int player_id;  // 플레이어의 id
    int timer;  // 게임 총 진행 시간 
}st_pkt;

typedef struct{
    int clnt_sock;  // 스레드에게 넘길 클라이언트 소켓 디스크립션
    int player_num;  // 플레이어 수 
    int size;  // 게임 사이즈
    int timer;  // 게임 총 진행 시간 
    int player_id;  // 플레이어의 id
}thread_arg;


game_state **matrix;
c_pkt *players;

struct timespec start, end;

int op_count = 0;

void *handle_clnt(void *arg);

int main(int argc, char *argv[])
{
    int player_num; // 플레이어의 수
    int size; // 게임 사이즈 
    int board; // 판의 개수
    int timer;  // 게임 총 진행 시간
    int port; // 서버의 포트 번호
    int opt;
    extern char *optarg;
    srand((unsigned int)time(NULL));

    while ((opt = getopt(argc, argv, "n:s:b:t:p:")) != -1) {
        switch (opt) {
            case 'n':
                player_num = atoi(optarg);
                break;
            case 's':
                size = atoi(optarg);
                break;
            case 'b':
                board = atoi(optarg); 
                break;
            case 't':
                timer = atoi(optarg); 
                break;
            case 'p':
                port = atoi(optarg); 
                break;
            case '?':
                printf("알 수 없는 옵션입니다..\n");
                return 1;
        }

    }
    if (size < 0 || board < 0 || board*2 > (size*size) || timer < 0) {
        printf("인자가 올바르지 않습니다.\n");
        return 1;
    }

    thread_arg* arg = (thread_arg*)malloc(sizeof(thread_arg));
    arg->player_num = player_num;
    arg->timer = timer;
    arg->size = size;

    matrix = (game_state **)malloc(sizeof(game_state *) * size);
    for(int i = 0; i < size; i++){
        matrix[i] = (game_state *)malloc(sizeof(game_state) * size);
    }

    int x,y; 

    // 게임 초기값 설정
    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            matrix[i][j].player = 0;
            matrix[i][j].pillow = 0;
        }
    }

    players = (c_pkt *)malloc(sizeof(c_pkt) * player_num);

    // 플레이어 수 만큼 위치 랜덤 배정 
    int count = 0;
    while(count < player_num){
        x = (rand() % size);
        y = (rand() % size);
        if(matrix[y][x].player == 0){
            matrix[y][x].player = count + 1;
            players[count].x = x;
            players[count].y = y;
            players[count].pillow = 0;
            count++;
        }
    }

    // 판 개수 만큼 Red팀 판 설정 
    count = 0;
    while(count < board){
        x = (rand() % size);
        y = (rand() % size);
        if(matrix[y][x].pillow == 0){
            matrix[y][x].pillow = 1;
            count++;
        }
    }

    // 판 개수 만큼 Blue팀 판 설정 
    count = 0;
    while(count < board){
        x = (rand() % size);
        y = (rand() % size);
        if(matrix[y][x].pillow == 0){
            matrix[y][x].pillow = 2;
            count++;
        }
    }

	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
  
	pthread_mutex_init(&mutx, NULL);
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET; 
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(port);
	
	if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");
	
	while (1){
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);
		
		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_cnt++] = clnt_sock;
		pthread_mutex_unlock(&mutx);
        
        arg->clnt_sock = clnt_sock;
        arg->player_id = clnt_cnt;
		pthread_create(&t_id, NULL, handle_clnt, (void*)arg);
		pthread_detach(t_id);
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
        if(clnt_cnt == player_num){
            clock_gettime(CLOCK_MONOTONIC, &start);
        }
	}

    close(serv_sock);
	return 0;
}

void *handle_clnt(void * arg){
    thread_arg* init = (thread_arg*)arg;
	int clnt_sock = init->clnt_sock;
    int size = init->size;
    int player_num = init->player_num;
    int timer = init->timer;
    int player_id = init->player_id;
	int str_len = 0, i, read_cnt, recv_len, index;
    int x, y;

	s_pkt *send_pkt = malloc(sizeof(s_pkt));
    c_pkt *recv_pkt = malloc(sizeof(c_pkt));
    st_pkt *start_pkt = malloc(sizeof(st_pkt));
    game_state *info = malloc(sizeof(game_state));

    start_pkt->size = size;
    start_pkt->player_num = player_num;
    start_pkt->player_id = player_id;
    start_pkt->timer = timer;

    while(1){
        if(clnt_cnt == player_num){
            break;
        }
    }



    printf("all player come\n");

    write(clnt_sock, start_pkt, sizeof(st_pkt));

    // 초기 값 클라이언트에게 전송 
    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            memset(info, 0, sizeof(game_state));
            info->player = matrix[i][j].player;
            info->pillow = matrix[i][j].pillow;
            write(clnt_sock, info, sizeof(game_state));
        }
    }

    x = players[player_id - 1].x;
    y = players[player_id - 1].y;

    //게임 시작
    while(1){
        memset(recv_pkt, 0, sizeof(c_pkt));
        recv_len = 0;
        // 키 입력 받기 
        while (recv_len < sizeof(c_pkt)) {
            read_cnt = read(clnt_sock, (char*)recv_pkt + recv_len, sizeof(c_pkt) - recv_len);
            if (read_cnt == -1){
                error_handling("read() error");
            }
            else if (read_cnt == 0){
                break; 
            } 	
            recv_len += read_cnt;
        }
        players[player_id - 1].pillow = 0;

        //enter키 입력으로 판 뒤집기 
        pthread_mutex_lock(&mutx);
        if(recv_pkt->pillow == 1){
            if(matrix[y][x].pillow == 1){
                matrix[y][x].pillow = 2;
            }
            else if(matrix[y][x].pillow == 2){
                matrix[y][x].pillow = 1;
            }
            players[player_id - 1].pillow = 1;
        }
        // 그 외 wasd입럭 적용히기 
        else{
            matrix[y][x].player = 0;
            matrix[recv_pkt->y][recv_pkt->x].player = player_id;
            players[player_id - 1].x = recv_pkt->x;
            players[player_id - 1].y = recv_pkt->y;
        }
        pthread_mutex_unlock(&mutx);

        memset(send_pkt, 0, sizeof(s_pkt));

        pthread_mutex_lock(&mutx);
        op_count++;
        pthread_mutex_unlock(&mutx);

        while(1){
            if(op_count % player_num == 0){
                break;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

        double elapsed = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec) * 1e-9;

        // 각 클라이언트 값을 합쳐서 전송 
        for(int i = 0; i < player_num; i++){
            if(players[i].pillow == 1){
                send_pkt->x = -1;
            }
            else{
                send_pkt->x = players[i].x;
                send_pkt->y = players[i].y;
            }
            send_pkt->clock = timer - elapsed;
            send_pkt->player_id = i + 1;
            write(clnt_sock, send_pkt, sizeof(s_pkt));
        }

        if(timer - elapsed <= 0){
            break;
        }
    }

    // 게임 결과 전송 
    printf("게임종료n");

    int red = 0;
    int blue = 0;
    
    game_state *end_pkt = malloc(sizeof(game_state));

    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            if(matrix[i][j].pillow == 1){
                red++;
            }
            else if(matrix[i][j].pillow == 2){
                blue++;
            }
        }
    }

    end_pkt->player = red;
    end_pkt->pillow = blue;

    write(clnt_sock, end_pkt, sizeof(game_state));


	pthread_mutex_lock(&mutx);
	for (i = 0; i < clnt_cnt; i++){
		if (clnt_sock == clnt_socks[i]){
			while (i++ < clnt_cnt-1)
				clnt_socks[i] = clnt_socks[i+1];
			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);
	close(clnt_sock);
	free(send_pkt);
    free(start_pkt);
    free(recv_pkt);
    free(end_pkt);
    free(info);
	printf("Disconnected client Num: %d \n", clnt_sock);
	return NULL;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}