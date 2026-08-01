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
#define COLOR_RESET	"\033[0m"
void error_handling(char *message);

typedef struct{
	unsigned int data_type; // 0: Pkt, 1: END
	unsigned int data_size;
	unsigned char data[BUF_SIZE];
}pkt_t;

int main(int argc, char *argv[])
{
	int sd;
	pkt_t *recv_pkt = malloc(sizeof(pkt_t));
    pkt_t *send_pkt = malloc(sizeof(pkt_t));
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

    while (read(0, &c, sizeof(c)) > 0){
        memset(recv_pkt, 0, sizeof(pkt_t));
		memset(send_pkt, 0, sizeof(pkt_t));

        if(c == 127){
            printf("\r%sSearch Word: %s%s", COLOR_GREEN, word, COLOR_RESET);
            index--;
            word[index] = '\0';
            printf("\b \b");
        }
        else if(c == 10){
            printf("\n");
            term.c_lflag |= ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &term);
            send_pkt->data_type = 1;
            write(sd, send_pkt, sizeof(pkt_t));
            break;
        }
        else{
            word[index] = c;
            printf("\r%sSearch Word:%s%s", COLOR_GREEN, COLOR_RESET, word);
            index++;
        }

        send_pkt->data_type = 0;
        send_pkt->data_size = strlen(word);
        sprintf(send_pkt->data, "%s", word);
        write(sd, send_pkt, sizeof(pkt_t));

        printf("\x1b[%dB", 1);
        printf("\r---------------------------------");
        printf("\n");

        for(int i = 0; i < count; i++){
        printf("\r\x1b[2K"); 
        printf("\n");
        }
        printf ("\x1b[%dA", count + 1);

        count = 0;

        while(1){
            recv_len = 0;
            while (recv_len < sizeof(pkt_t)) {
                read_cnt = read(sd, (char*)recv_pkt + recv_len, sizeof(pkt_t) - recv_len);
                if (read_cnt == -1){
                    error_handling("read() error");
                }
                else if (read_cnt == 0){
                    break; 
                } 	
                recv_len += read_cnt;
		    }

            if (recv_pkt->data_type == 1){
			    break; 
    	    }
            printf("\n");
            printf("\r%d. ", count + 1);
            for(int i = 0; i < strlen(recv_pkt->data); i++){
                if(strncmp(recv_pkt->data + i, word, strlen(word)) == 0){
                    printf("%s%s%s", COLOR_ORANGE, word, COLOR_RESET);
                    i += strlen(word) - 1;
                    continue;
                }
                printf("%c", recv_pkt->data[i]);
            }   
            count++;
            usleep(100000);
        }
        printf ("\x1b[%dA", count+1);


        fflush(stdout);
        sleep(0.1);
	}
	
	
	close(sd);
	free(recv_pkt);
    free(send_pkt);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}