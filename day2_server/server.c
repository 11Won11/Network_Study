#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30
#define END 2
void error_handling(char *message);

typedef struct{
	unsigned int seq;
	unsigned int data_type; // 0: Pkt, 1: ACK 2: END
	unsigned int file_size;
	unsigned char file[BUF_SIZE];
}pkt_t;

int main(int argc, char *argv[])
{
	FILE* fp;
	int serv_sock;
	int str_len, read_cnt;
	char filename[BUF_SIZE];
	socklen_t clnt_adr_sz;
	pkt_t *send_pkt = malloc(sizeof(pkt_t));
	pkt_t *recv_pkt = malloc(sizeof(pkt_t));
	int total_count = 0;
	int flag = 0; // 0: normal, 1: Pkt loss
	

	struct sockaddr_in serv_adr, clnt_adr;
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (serv_sock == -1)
		error_handling("UDP socket creation error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET; 
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	
	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

	clnt_adr_sz = sizeof(clnt_adr);
	str_len = recvfrom(serv_sock, filename, BUF_SIZE, 0, 
							(struct sockaddr*)&clnt_adr, &clnt_adr_sz);
						
	fp = fopen(filename, "rb");
	if(fp == NULL){
		error_handling("open() error!!");
	}
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	printf("file size check: %d\n", size);
	fseek(fp, 0, SEEK_SET);

	struct timeval optVal = {3, 0};
	int optLen = sizeof(optVal);
	setsockopt(serv_sock, SOL_SOCKET, SO_RCVTIMEO, &optVal, optLen);

	int sequence = 0;

	while(1){

		if(total_count >= size){
			send_pkt->data_type = END;
			sendto(serv_sock, send_pkt, sizeof(pkt_t), 0, 
								(struct sockaddr*)&clnt_adr, clnt_adr_sz);
			break;
		}
		if(flag == 0){
			read_cnt = fread((void*)send_pkt->file, sizeof(char), BUF_SIZE, fp);
			total_count += read_cnt;
		}
		send_pkt->data_type = 0;
		send_pkt->file_size = read_cnt;
		send_pkt->seq = sequence;
		
		printf("seq no: %d\n file: %s\n", send_pkt->seq, send_pkt->file);

		sendto(serv_sock, send_pkt, sizeof(pkt_t), 0, 
								(struct sockaddr*)&clnt_adr, clnt_adr_sz);

		str_len = recvfrom(serv_sock, recv_pkt, sizeof(pkt_t), 0, 
								(struct sockaddr*)&clnt_adr, &clnt_adr_sz);
		
		if(str_len == -1){
			flag = 1;
			continue;
		}
		flag = 0;
		sequence++;
	}
	puts("Send file data");

	fclose(fp);
	free(recv_pkt);
	free(send_pkt);

	close(serv_sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
