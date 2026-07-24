#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/epoll.h>

#define BUF_SIZE 30
#define EPOLL_SIZE 50
void error_handling(char *buf);

int main(int argc, char *argv[])
{
    FILE* fp;
    char buf[BUF_SIZE];
    int read_cnt;
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;

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
            }
            else    // read message!
            {
                str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);
    
                fp = fopen(buf, "rb"); 
                if(fp == NULL){
                    error_handling("open() error!!");
                }
                while(1)
                {
                    read_cnt = fread((void*)buf, 1, BUF_SIZE, fp);
                    if (read_cnt < BUF_SIZE)
                    {   
                        write(ep_events[i].data.fd, buf, read_cnt);
                        break;
                    }
                    write(ep_events[i].data.fd, buf, BUF_SIZE);
                }
                fclose(fp);
                
                shutdown(ep_events[i].data.fd, SHUT_WR);	
                read(ep_events[i].data.fd, buf, BUF_SIZE);
                printf("Message from client: %s \n", buf);
                
                epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                close(ep_events[i].data.fd);
                printf("closed client: %d \n", ep_events[i].data.fd);
                memset(buf, 0, sizeof(buf));
            }
		}
	}
	close(serv_sock);
	return 0;
}

void error_handling(char *buf)
{
	fputs(buf, stderr);
	fputc('\n', stderr);
	exit(1);
}  