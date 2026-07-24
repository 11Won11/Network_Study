#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
void error_handling(char *message);

typedef struct{
	unsigned int data_type; // 0: Pkt, 1: END
	unsigned int file_size;
	unsigned char file[BUF_SIZE];
}pkt_t;

int main(int argc, char *argv[])
{
	int sd;
	FILE *fp;
	pkt_t *recv_pkt = malloc(sizeof(pkt_t));
	
	int read_cnt, recv_len;
	struct sockaddr_in serv_adr;
	if (argc != 3) {
		printf("Usage: %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	
	fp = fopen("picture.png", "wb");
	sd = socket(PF_INET, SOCK_STREAM, 0);   

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
	
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
		
		if (read_cnt == 0){
			break; 
		}
		fwrite((void*)recv_pkt->file, sizeof(char), recv_pkt->file_size, fp);
		
		if (recv_pkt->data_type == 1){
			break; 
    	}
	}
	sleep(6);
	
	
	puts("Received file data");
	fclose(fp);
	close(sd);
	free(recv_pkt);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
