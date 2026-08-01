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
#define MAX_CLNT 256

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

void error_handling(char *message);

typedef struct{
	unsigned int data_type; // 0: Pkt, 1: END
	unsigned int data_size;
	unsigned char data[BUF_SIZE];
}pkt_t;

typedef struct{
	unsigned int word_count;
	unsigned char word[BUF_SIZE];
}word_data;

void *handle_clnt(void *arg);
int find_word(word_data (*arr), char* data);
int compare(const void *a, const void *b);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
	if (argc != 3) {
		printf("Usage : %s <port> <filename>\n", argv[0]);
		exit(1);
	}
  
	pthread_mutex_init(&mutx, NULL);
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET; 
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	
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
	
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
		pthread_detach(t_id);
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
	}
    close(serv_sock);
	return 0;
}

void *handle_clnt(void * arg){
	int clnt_sock = *((int*)arg);
	int str_len = 0, i, read_cnt, recv_len, index;

	pkt_t *send_pkt = malloc(sizeof(pkt_t));
    pkt_t *recv_pkt = malloc(sizeof(pkt_t));
    word_data arr[10]; 

    while(1){
		memset(recv_pkt, 0, sizeof(pkt_t));
		memset(send_pkt, 0, sizeof(pkt_t));
		memset(arr, 0, sizeof(arr));

        recv_len = 0;
    
		while (recv_len < sizeof(pkt_t)) {
			read_cnt = read(clnt_sock, (char*)recv_pkt + recv_len, sizeof(pkt_t) - recv_len);
			if (read_cnt == -1){
				error_handling("read() error");
			}
			else if (read_cnt == 0){
				break; 
			} 	
			recv_len += read_cnt;
		}

        printf("%s\n", recv_pkt->data);
        
        if (recv_pkt->data_type == 1){
			break; 
    	}
        index = find_word(arr, recv_pkt->data);
        for(int i = 0; i < index; i++){
            printf("word: %s\n", arr[i].word);
            printf("count: %d\n", arr[i].word_count);
			send_pkt->data_size = strlen(arr[i].word);
			send_pkt->data_type = 0;
			sprintf(send_pkt->data, "%s", arr[i].word);
			write(clnt_sock, send_pkt, sizeof(pkt_t));
        }
		send_pkt->data_type = 1;
		write(clnt_sock, send_pkt, sizeof(pkt_t));
    }

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
    free(recv_pkt);
	printf("Disconnected client Num: %d \n", clnt_sock);
	return NULL;
}

int find_word(word_data (*arr), char *data){
    FILE* fp;
    char buf[BUF_SIZE];
    char tmp[BUF_SIZE];
    char count[BUF_SIZE];
    int index = 0;
    int check, flag, comp = 0;

    fp = fopen("data.txt", "r");
    if(fp == NULL){
        error_handling("open() error!!");
    }

    while(!feof(fp)){
        memset(tmp, 0, sizeof(tmp));
        memset(count, 0, sizeof(count));
        fgets(buf, BUF_SIZE, fp);
        check = 0;
        flag = 0;
		for(int i = 0; i < strlen(buf); i++){
			if(strncmp(buf + i, data, strlen(data)) == 0 && (strlen(data) != 0)){
            	printf("%s\n", buf);
				for(int i = 0; i < strlen(buf); i++){
					//printf("check1: %s\n", tmp);
					if(buf[i] == 32 && buf[i + 1] == 32){
						i++;
						flag = 1;
					}
					if(flag == 0){
						tmp[i] = buf[i];
					}
					else{
						count[check] = buf[i];
						check++;
					}
				}
				if(arr[index].word_count >= atoi(count) && (comp == 1)){
					continue;
				}
				sprintf(arr[index].word, "%s", tmp);
				arr[index].word_count = atoi(count);
				
				if(index <= 9){
					index++;
				} 
				else{
					comp = 1;
				}
        	}
        qsort((word_data*)arr, index, sizeof(word_data), compare);
		}
    }
    fclose(fp);
    return index;

}

int compare(const void *a, const void *b){
    return ((word_data *)b)->word_count - ((word_data *)a)->word_count;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}