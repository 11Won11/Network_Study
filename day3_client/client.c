#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 64
void error_handling(char *message);

typedef struct{
    unsigned int data_type; // 0: NORM 1: END 2: ERROR
	unsigned int file_size;
	unsigned char file[BUF_SIZE];
}pkt_t;


int main(int argc, char *argv[])
{
	int sd;
	FILE *fp;
    int menu;
    char sen_menu[4];
	char buf[BUF_SIZE];
    char filename[BUF_SIZE];
	int read_cnt, str_len, recv_len, recv_cnt;
	struct sockaddr_in serv_adr;
    pkt_t *send_pkt, *recv_pkt;
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

    send_pkt = (pkt_t*)malloc(sizeof(pkt_t));
    recv_pkt = (pkt_t*)malloc(sizeof(pkt_t));

    while(1){
        memset(send_pkt, 0, sizeof(pkt_t));
        memset(recv_pkt, 0, sizeof(pkt_t));
        printf("Menu(1. Print File, 2. Move Dir, 3. Select & Copy file, 4. Upload file, 5. Quit): ");
        if(scanf("%d", &menu) != 1){
            printf("잘못된 입력입니다. 정수를 입력해주세요.\n");
            while (getchar() != '\n'); 
            continue;
        }
        if(menu < 1 || menu > 5){
            printf("존재하지 않는 명령어입니다.\n");
            continue;
        }

        sprintf(sen_menu, "%d", menu);
        write(sd, sen_menu, 4);
        
        if(menu == 1){
            read_cnt = read(sd, recv_pkt, sizeof(pkt_t));
            printf("Print Current Working Dir: ");
			for(int i = 0; i < recv_pkt->file_size; i++){
				printf("%c", recv_pkt->file[i]);
			}	
			printf("\n");
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
                if(recv_pkt->data_type == 2){
                    printf("Open Dir failed\n");
                    break;
                }

                if (recv_pkt->data_type == 1){
                    break; 
                }

                printf("%s / File size: %d\n", recv_pkt->file, recv_pkt->file_size);
            }
        }
        else if(menu == 2){
            read_cnt = read(sd, recv_pkt, sizeof(pkt_t));
            printf("Print Current Working Dir: ");
			for(int i = 0; i < recv_pkt->file_size; i++){
				printf("%c", recv_pkt->file[i]);
			}	
			printf("\n");

            printf("이동하고 싶은 디렉토리 주소를 입력하세요: ");
            scanf("%s", send_pkt->file);

            send_pkt->file_size = strlen(send_pkt->file);
            write(sd, send_pkt, sizeof(pkt_t));   

            read_cnt = read(sd, recv_pkt, sizeof(pkt_t));
            if(recv_pkt->data_type == 1){
                puts("Change Dir!");
            }
            else{
                puts("Dir error!!");
            }
        }
        else if(menu == 3){
            printf("Input file name: ");
	        scanf("%s", filename);  

            fp = fopen(filename, "wb");
            if(fp == NULL){
                error_handling("open() error!!");
            }

            sprintf(send_pkt->file, "%s", filename);
            send_pkt->data_type = 0;
            send_pkt->file_size = strlen(filename);

            write(sd, send_pkt, sizeof(pkt_t));
            
            while(1){
                read_cnt = read(sd, recv_pkt, sizeof(pkt_t));
                if(recv_pkt->data_type == 1){
                    fwrite((void*)recv_pkt->file, sizeof(char), recv_pkt->file_size, fp);
                    break;
                }
                fwrite((void*)recv_pkt->file, sizeof(char), recv_pkt->file_size, fp);
            }
            puts("Received file data");
            fclose(fp);
        }
        else if(menu == 4){
            printf("Input file name: ");
	        scanf("%s", filename);  

            fp = fopen(filename, "rb");
            if(fp == NULL){
                error_handling("open() error!!");
            }

            sprintf(send_pkt->file, "%s", filename);
            send_pkt->data_type = 0;
            send_pkt->file_size = strlen(filename);

            write(sd, send_pkt, sizeof(pkt_t));

             while(1){
                read_cnt = fread((void*)send_pkt->file, sizeof(char), BUF_SIZE, fp);
                send_pkt->file_size = read_cnt;
                send_pkt->data_type = 0;
                if (read_cnt < BUF_SIZE)
                {   
                    send_pkt->data_type = 1;
                    write(sd, send_pkt, sizeof(pkt_t));
                    send_pkt->data_type = 0;
                    break;
                }
                write(sd, send_pkt, sizeof(pkt_t));
            }
            fclose(fp);
            
            puts("Send File Data");

        }
        else{
            break;
        }
    }
    free(send_pkt);
    free(recv_pkt);
	close(sd);
    
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}