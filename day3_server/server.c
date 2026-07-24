#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <dirent.h>

#define BUF_SIZE 64
#define EPOLL_SIZE 50

typedef struct{
    unsigned int data_type; // 0: NORM 1: END 2: ERROR
	unsigned int file_size;
	unsigned char file[BUF_SIZE];
}pkt_t;

void error_handling(char *buf);
void search_directory(char (*dir_path)[BUF_SIZE], int clnt_sock, pkt_t *send_pkt);

int main(int argc, char *argv[])
{
    FILE* fp;
    char buf[BUF_SIZE];
    char menu[4];
    int int_menu, dir_check;
    int read_cnt;
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
    char dir_path[EPOLL_SIZE][BUF_SIZE];
    pkt_t *send_pkt, *recv_pkt;    

    struct epoll_event *ep_events;
    struct epoll_event event;
    int epfd, event_cnt;

	socklen_t adr_sz;
	int fd_max, str_len, fd_num, i;
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	
	if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

    epfd = epoll_create(EPOLL_SIZE);
    ep_events = malloc(sizeof(struct epoll_event)*EPOLL_SIZE);

    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

    send_pkt = (pkt_t*)malloc(sizeof(pkt_t));
    recv_pkt = (pkt_t*)malloc(sizeof(pkt_t));

	while (1)
	{
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
    
        if(event_cnt == -1){
            puts("epoll_wait() error");
            break;
        }

		for (i = 0; i < event_cnt; i++){
            if (ep_events[i].data.fd == serv_sock)     
            {
                adr_sz = sizeof(clnt_adr);
                clnt_sock =
                    accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                printf("connected client: %d \n", clnt_sock);
                sprintf(dir_path[clnt_sock], "%s", "/server/day3_server");
            }
            else    
            {

                str_len = read(ep_events[i].data.fd, menu, 4);
                int_menu = atoi(menu);
                printf("int menu: %d\n", int_menu);

                if(int_menu == 1){
                    dir_check = chdir(dir_path[ep_events[i].data.fd]);  
                    if(getcwd(dir_path[ep_events[i].data.fd], BUF_SIZE) == NULL){
                        error_handling("dir path error!!");
                    }
                    sprintf(send_pkt->file, "%s", dir_path[ep_events[i].data.fd]);
                    send_pkt->file_size = strlen(send_pkt->file);
                    write(ep_events[i].data.fd, send_pkt, sizeof(pkt_t));
                    search_directory(dir_path, ep_events[i].data.fd, send_pkt);
                }
                else if(int_menu == 2){
                    
                    if(getcwd(dir_path[ep_events[i].data.fd], BUF_SIZE) == NULL){
                        error_handling("dir path error!!");
                    }
                    sprintf(send_pkt->file, "%s", dir_path[ep_events[i].data.fd]);
                    send_pkt->file_size = strlen(send_pkt->file);
                    write(ep_events[i].data.fd, send_pkt, sizeof(pkt_t));

                    read(ep_events[i].data.fd, recv_pkt, sizeof(pkt_t)); 

                    dir_check = chdir(recv_pkt->file);
                    sprintf(dir_path[ep_events[i].data.fd], "%s", recv_pkt->file);
                    printf("dir path: %s\n", dir_path[ep_events[i].data.fd]);

                    if(dir_check == 0){
                        puts("Change Dir!");
                        send_pkt->data_type = 1;
                        write(ep_events[i].data.fd, send_pkt, sizeof(pkt_t));
                        send_pkt->data_type = 0;
                        if(getcwd(dir_path[ep_events[i].data.fd], BUF_SIZE) == NULL){
                            error_handling("dir path error!!");
                        }
                    }
                    else{
                        puts("Dir error!!");
                        send_pkt->data_type = 2;
                        write(ep_events[i].data.fd, send_pkt, sizeof(pkt_t));
                    }
                }

                else if(int_menu == 3){
                    dir_check = chdir(dir_path[ep_events[i].data.fd]);  

                    read(ep_events[i].data.fd, recv_pkt, sizeof(pkt_t));
        
                    fp = fopen(recv_pkt->file, "rb"); 
                    if(fp == NULL){
                        error_handling("open() error!!");
                    }

                    while(1)
                    {
                        read_cnt = fread((void*)send_pkt->file, sizeof(char), BUF_SIZE, fp);
                        send_pkt->file_size = read_cnt;
                        send_pkt->data_type = 0;
                        if (read_cnt < BUF_SIZE)
                        {   
                            send_pkt->data_type = 1;
                            write(ep_events[i].data.fd, send_pkt, sizeof(pkt_t));
                            send_pkt->data_type = 0;
                            break;
                        }
                        write(ep_events[i].data.fd, send_pkt, sizeof(pkt_t));
                    }
                    fclose(fp);
                    
                    puts("Send File Data");
                }
                else if(int_menu == 4){
                    dir_check = chdir(dir_path[ep_events[i].data.fd]);  

                    read(ep_events[i].data.fd, recv_pkt, sizeof(pkt_t));
        
                    fp = fopen(recv_pkt->file, "wb"); 
                    if(fp == NULL){
                        error_handling("open() error!!");
                    }

                    while(1){
                        read_cnt = read(ep_events[i].data.fd, recv_pkt, sizeof(pkt_t));
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
                    epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                    close(ep_events[i].data.fd);
                    printf("closed client: %d\n", ep_events[i].data.fd);
                }
            }
            
        }
	
	}
    free(send_pkt);
    free(recv_pkt);
	close(serv_sock);
	return 0;
}

void search_directory(char (*dir_path)[BUF_SIZE], int clnt_sock, pkt_t *send_pkt){
    printf("%s\n", dir_path[clnt_sock]);
    DIR* dp = opendir(dir_path[clnt_sock]);
    FILE *fp;
	int index = 0;
    int size;
    if (dp == NULL){
        perror("opendir failed");
        send_pkt->data_type = 2;
        write(clnt_sock, send_pkt, sizeof(pkt_t));
        return;
    }

    struct dirent* entry;

    while((entry = readdir(dp)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
		}
        
        if(entry->d_type == DT_DIR){
            sprintf(send_pkt->file, "Dir name: %s", entry->d_name);
            send_pkt->file_size = 0;
            send_pkt->data_type = 0;
        }
        else if(entry->d_type == DT_REG) {
            fp = fopen(entry->d_name, "rb");
            if (fp == NULL){
                 error_handling("open() error!!");
            }    
            fseek(fp, 0, SEEK_END);
            size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            printf("파일 크기: %d 바이트\n", size);

            fclose(fp);
            sprintf(send_pkt->file, "File name: %s", entry->d_name);
            send_pkt->file_size = size;
            send_pkt->data_type = 0;
        }
        write(clnt_sock, send_pkt, sizeof(pkt_t));
	}
	send_pkt->data_type = 1;
	write(clnt_sock, send_pkt, sizeof(pkt_t));
    closedir(dp);
}

void error_handling(char *buf)
{
	fputs(buf, stderr);
	fputc('\n', stderr);
	exit(1);
}  

