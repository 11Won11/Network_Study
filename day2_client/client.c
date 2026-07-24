#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

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
	int sock;
	char message[BUF_SIZE];
	int str_len;
	socklen_t adr_sz;
	pkt_t *send_pkt = malloc(sizeof(pkt_t));
	pkt_t *recv_pkt = malloc(sizeof(pkt_t));
	int total_size = 0;
	struct timespec start, end;
	int flag = 0;

	struct sockaddr_in serv_adr, from_adr;
	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	
	sock = socket(PF_INET, SOCK_DGRAM, 0);   
	if (sock == -1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));
	
	fputs("Input file name: ", stdout);
	scanf("%s", message);  
	
	sendto(sock, message, strlen(message), 0, 
				(struct sockaddr*)&serv_adr, sizeof(serv_adr));
	
	clock_gettime(CLOCK_MONOTONIC, &start);
	
	fp = fopen(message, "wb");

	send_pkt->data_type = 1;
	send_pkt->file_size = 0;
	
	struct timeval optVal = {2, 0};
	int optLen = sizeof(optVal);
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &optVal, optLen);

	while(1){
		str_len = recvfrom(sock, recv_pkt, sizeof(pkt_t), 0, 
				(struct sockaddr*)&from_adr, &adr_sz);

		if(str_len == -1){
			continue;
		}
			
		if(recv_pkt->data_type == END){
			clock_gettime(CLOCK_MONOTONIC, &end);
			break;
		}

		if(recv_pkt->seq == 20 && flag == 0){
			sleep(6);
			flag = 1;
			continue;
		}

		total_size += recv_pkt->file_size;
		fwrite((void*)recv_pkt->file, sizeof(char), recv_pkt->file_size, fp);

		send_pkt->seq = recv_pkt->seq;
		
		sendto(sock, send_pkt, sizeof(send_pkt), 0, 
					(struct sockaddr*)&serv_adr, sizeof(serv_adr));
	}
	puts("Received file dat\n");
	double time = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	double Bps = total_size/time;
	printf("time: %lf\n", time);
	printf("Throughput = %0.2lfBps\n", Bps);

	fclose(fp);
	free(recv_pkt);
	free(send_pkt);

	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}