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

#define BUF_SIZE 1024
#define COLOR_GREEN	"\033[38;2;204;255;204m"
#define COLOR_ORANGE	"\033[38;2;255;153;051m"
#define COLOR_RED "\033[41m"
#define COLOR_BLUE "\033[44m"
#define COLOR_RESET	"\033[0m"
void error_handling(char *message);

typedef struct{
    int x, y; // 좌표 값
    int clock; // 현재 경과된 시간 
    int player_id; // 각 플레이어의 번호 없을 시 0
}s_pkt;   

typedef struct{
    int x, y;  // 좌표 값, x가 음수일 경우 enter키 입력 
    int pillow; // 0: default, 1: enter키 입력 
}c_pkt;

typedef struct{
    int player; // 각 플레이어의 번호 없을 시 0
    int pillow; // 0: no pillow, 1: Red, 2: Blue
}game_state;

typedef struct{
    int player_num;  // 플레이어의 수 
    int size;  // 게임 사이즈
    int player_id;  // 플레이어의 id
    int timer;  // 게임 총 진행 시간 
}st_pkt;

game_state **matrix;
c_pkt *players;

int main(int argc, char *argv[])
{
	int sd;
    struct termios term;
    char word[BUF_SIZE];
    char c;
    int index = 0, count;

    tcgetattr(STDIN_FILENO, &term);
	term.c_lflag &= ~ICANON;    
	term.c_lflag &= ~ECHO;     
	term.c_cc[VMIN] = 1;        
	term.c_cc[VTIME] = 0;     
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
	
	int read_cnt, recv_len;
	struct sockaddr_in serv_adr;
	if (argc != 3) {
		printf("Usage: %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	
	sd = socket(PF_INET, SOCK_STREAM, 0);   

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
    memset(word, 0, sizeof(word));

    s_pkt *recv_pkt = malloc(sizeof(s_pkt));
    c_pkt *send_pkt = malloc(sizeof(c_pkt));
    st_pkt *start_pkt = malloc(sizeof(st_pkt));
    game_state *info = malloc(sizeof(game_state));

    recv_len = 0;

    // 시작 정보 받기 
    while (recv_len < sizeof(st_pkt)) {
        read_cnt = read(sd, (char*)start_pkt + recv_len, sizeof(st_pkt) - recv_len);
        if (read_cnt == -1){
            error_handling("read() error");
        }
        else if (read_cnt == 0){
            break; 
        } 	
        recv_len += read_cnt;
    }

    // 받은 정보로 변수 선언 
    int size = start_pkt->size;
    int player_num = start_pkt->player_num;
    int timer = start_pkt->timer;
    int player_id = start_pkt->player_id;

    matrix = (game_state **)malloc(sizeof(game_state *) * size);
    for(int i = 0; i < size; i++){
        matrix[i] = (game_state *)malloc(sizeof(game_state) * size);
    }

    players = (c_pkt *)malloc(sizeof(c_pkt) * player_num);

    // 초기값 가져오기 
    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            memset(info, 0, sizeof(game_state));
            recv_len = 0;
            while (recv_len < sizeof(game_state)) {
                read_cnt = read(sd, (char*)info + recv_len, sizeof(game_state) - recv_len);
                if (read_cnt == -1){
                    error_handling("read() error");
                }
                else if (read_cnt == 0){
                    break; 
                } 	
                recv_len += read_cnt;
            }
            if(matrix[i][j].player != 0){
                players[matrix[i][j].player - 1].x = j;
                players[matrix[i][j].player - 1].y = i;
            }
            matrix[i][j].player = info->player;
            matrix[i][j].pillow = info->pillow;
        }
    }

    // 게임 시작 
    while(1){
        // 터미널에 게임 정보 출력 
        for(int i = 0; i < size; i++){
            for(int j = 0; j < size; j++){
                if(matrix[i][j].pillow == 1){
                    printf("|%s %d %s|", COLOR_RED, matrix[i][j].player, COLOR_RESET);
                }
                else if(matrix[i][j].pillow == 2){
                    printf("|%s %d %s|", COLOR_BLUE, matrix[i][j].player, COLOR_RESET);
                }
                else{
                    printf("| %d |", matrix[i][j].player);
                }
            }
            printf("\n");
        }

        while (read(0, &c, sizeof(c)) > 0){
		    memset(send_pkt, 0, sizeof(c_pkt));
            
            //기본 값 입력 
            send_pkt->x = players[player_id -1].x;
            send_pkt->x = players[player_id -1].y;
            send_pkt->pillow = 0;

            // w입력 
            if(c == 119){
                if(send_pkt->y != 0){
                    send_pkt->y = players[player_id -1].y - 1;
                }
                break;
            }
            // a입력
            else if(c == 97){
                if(send_pkt->x != 0){
                    send_pkt->x = players[player_id -1].x - 1;
                }
                break;
            }
            // s입력
            else if(c == 115){
                if(send_pkt->y != size){
                    send_pkt->y = players[player_id -1].y + 1;
                }
                break;
            }
            // d입력
            else if(c == 100){
                if(send_pkt->x != size){
                    send_pkt->x = players[player_id -1].x + 1;
                }
                break;
            }
            //enter입력
            else if(c == 10){
                send_pkt->pillow = 1;
                break;
            }
            // 그 외 키 입력 무시 
            else{
                continue;
            }
            puts("check1");
        }
        
        //입력 된 키 전송 
        write(sd, send_pkt, sizeof(c_pkt));

        int x, y;

        // 갱신된 정보 받기 
        for(int i = 0; i < player_num; i++){
            memset(recv_pkt, 0, sizeof(s_pkt));
            recv_len = 0;
            while (recv_len < sizeof(s_pkt)) {
                read_cnt = read(sd, (char*)recv_pkt + recv_len, sizeof(s_pkt) - recv_len);
                if (read_cnt == -1){
                    error_handling("read() error");
                }
                else if (read_cnt == 0){
                    break; 
                } 	
                recv_len += read_cnt;
            }

            x = players[i].x;
            y = players[i].y;

            if(recv_pkt->x < 0){
                if(matrix[y][x].pillow == 1){
                    matrix[y][x].pillow = 2;
                }
                else if(matrix[y][x].pillow == 2){
                    matrix[y][x].pillow = 1;
                }
            }
            else{
                matrix[y][x].player = 0;
                matrix[recv_pkt->y][recv_pkt->x].player = i+1;
                players[i].x = recv_pkt->x;
                players[i].y = recv_pkt->y;
            }

        }

        printf ("\x1b[%dA", size);
    }
	
    term.c_lflag != ~ECHO;  
    tcsetattr(STDIN_FILENO, TCSANOW, &term);

	close(sd);
	free(recv_pkt);
    free(send_pkt);
    free(start_pkt);
    free(info);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}