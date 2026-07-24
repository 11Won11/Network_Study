#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
#define BUF_SIZE_1 30
void error_handling(char *message);

typedef struct{
    unsigned int data_type; // 0: NORM 1: END 
	unsigned int file_size;
	unsigned char file[BUF_SIZE_1];
}pkt_t;

int main(int argc, char *argv[])
{
	FILE *fp;
	int sock;
	char message[BUF_SIZE];
	char buf[BUF_SIZE];
	int str_len, recv_len, recv_cnt, read_cnt;
	char file_size[BUF_SIZE_1];
	struct sockaddr_in serv_adr;
	char *file_data;
	pkt_t *recv_pkt = (pkt_t*)malloc(sizeof(pkt_t));

	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	
	sock = socket(PF_INET, SOCK_STREAM, 0);   
	if (sock == -1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("connect() error!");
	else
		puts("Connected...........");
	
	while (1) 
	{
		fputs("Menu(1. Print Dir, 2. Select & Copy file, 3. Quit): ", stdout);
		fgets(message, BUF_SIZE, stdin);
		
		if (!strcmp(message,"3\n"))
			break;

		str_len = write(sock, message, strlen(message));
		if (!strcmp(message,"1\n")){
			recv_len = 0;
			while (recv_len<str_len)
			{
				recv_cnt = read(sock, &message[recv_len], BUF_SIZE-1);
				if (recv_cnt == -1)
					error_handling("read() error!");
				recv_len+=recv_cnt;
			}
			
			message[recv_len]=0;
			printf("Print Dir: ");
			for(int i = 0; i < strlen(message); i++){
				if(message[i] == '\0'){
					break;
				}
				printf("%c", message[i]);
			}	
			printf("\n");
		}
		else if (!strcmp(message,"2\n")){
			int count = 0;
			fputs("Select file name: ", stdout);
			scanf("%s", message);
			getchar();

			str_len = write(sock, message, BUF_SIZE_1);
			printf("message length: %ld\n", strlen(message));
			read_cnt = read(sock, file_size, BUF_SIZE_1);
			fp = fopen(message, "wb");

			while(1){
                read_cnt = read(sock, recv_pkt, sizeof(pkt_t));
				printf("%s", recv_pkt->file);
                if(recv_pkt->data_type == 1){
                    fwrite((void*)recv_pkt->file, sizeof(char), recv_pkt->file_size, fp);
                    break;
                }
                fwrite((void*)recv_pkt->file, sizeof(char), recv_pkt->file_size, fp);
            }
            puts("Received file data");
            fclose(fp);
			
		}
		else{
			printf("존재하지 않는 명령어입니다.\n");
			continue;
		}
	}
	close(sock);
	
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
