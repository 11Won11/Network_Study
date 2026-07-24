#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dirent.h>

#define BUF_SIZE 1024
#define BUF_SIZE_1 256
#define BUF 30
void error_handling(char *message);

typedef struct{
    unsigned int data_type; // 0: NORM 1: END 
	unsigned int file_size;
	unsigned char file[BUF];
}pkt_t;

int search_directory(const char* dir_path, int max, int clnt_sock);

int main(int argc, char *argv[])
{
	int tmp;
	FILE *fp;
	int serv_sock, clnt_sock;
	char message[BUF_SIZE];
	char file_size[BUF_SIZE_1];
	char* filename;
	char* dir = "../day1_server"; 
	int str_len, i, read_cnt;
	pkt_t *send_pkt = (pkt_t*)malloc(sizeof(pkt_t));
	
	struct sockaddr_in serv_adr;
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz;
	
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);   
	if (serv_sock == -1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	clnt_adr_sz = sizeof(clnt_adr);
	clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
	if (clnt_sock == -1)
		error_handling("accept() error");
	else
		printf("Connected client\n");

	while ((str_len = read(clnt_sock, message, BUF_SIZE))!=0){
		if(!strncmp(message,"1\n", 1)){
			search_directory(dir, BUF_SIZE, clnt_sock);
		}
		else if(!strncmp(message,"2\n", 1)){
			
			str_len = read(clnt_sock, message, BUF_SIZE);
			printf("message: %s\n", message);
			printf("message buf length :%ld\n", strlen(message));
			filename = (char*)malloc(sizeof(char)*strlen(message));
			
			for(int i = 0; i < strlen(message); i++){
				filename[i] = message[i];
			}

			printf("file name: %s\n", filename);
			printf("size of filename: %ld\n", sizeof(filename));

			fp = fopen(filename, "rb");
			if(fp == NULL){
				error_handling("open() error!!");
			}

			while(1)
				{
					read_cnt = fread((void*)send_pkt->file, sizeof(char), BUF, fp);
					send_pkt->file_size = read_cnt;
					send_pkt->data_type = 0;
					printf("%s", send_pkt->file);
					if (read_cnt < BUF)
					{   
						send_pkt->data_type = 1;
						write(clnt_sock, send_pkt, sizeof(pkt_t));
						break;
					}
					write(clnt_sock, send_pkt, sizeof(pkt_t));
				}
			fclose(fp);
		
			printf("file send\n");
		}
		else if(!strncmp(message,"3\n", 1)){
			break;
		}
		else{
			continue;
		}
	}
	
	free(send_pkt);
	close(clnt_sock);
	close(serv_sock);
	
	return 0;
}

int search_directory(const char* dir_path, int max, int clnt_sock){
    DIR* dp = opendir(dir_path);
	int index = 0;
	char full_path[2048];
    if (dp == NULL){
        perror("opendir  failed");
        return 0;
    }

    struct dirent* entry;
    while((entry = readdir(dp)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
		}
        
		for(int i = 0; i < strlen(entry->d_name); i++){
			full_path[i + index] = entry->d_name[i];
		}
		index += strlen(entry->d_name);
		full_path[index] = ' ';
		index++;
	}
	printf("%s\n", full_path);
	full_path[index] = '\0';
	write(clnt_sock, full_path, strlen(full_path));
    closedir(dp);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
