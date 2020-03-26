/*
 * title        : linux socket client
 * name         : LEE YOUNG WOO
 * date         : 20.03.24
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MSG_LEN 256

void vf_error_handling(char * msg);

int main(int argc, char *argv[])
{
        if(argc != 2)
        {
                fprintf(stderr, "Usage : %s <port>\n", argv[0]);
                exit(1);
        }

        int clnt_sockfd;                // socket fd
        struct sockaddr_in serv_addr;   // AF_INET(IPv4)

        int fd_max, fd_num, i;

        struct timeval timeout;

        fd_set reads, cpy_reads;

        char send_msg[MSG_LEN], recv_msg[MSG_LEN];
        int send_msg_len, recv_msg_len, left_msg_len;

        if((clnt_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
                vf_error_handling("socket() error");

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        serv_addr.sin_port = htons(atoi(argv[1]));

        if(connect(clnt_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
                vf_error_handling("connect() error");

        fputs("Input Message(Q to quit) : \n", stdout);

        while(1)
        {
                fd_max = clnt_sockfd + 1;
                FD_ZERO(&reads);        // fd_set 테이블 초기화
                FD_SET(0, &reads);      // 0, 키보드로부터 읽기 사건이 발생하는지
                FD_SET(clnt_sockfd, &reads);

                timeout.tv_sec = 5;
                timeout.tv_usec = 500000;

                if(fd_num = select(fd_max, &reads, 0, 0, &timeout) == -1)
                        break;

                if(FD_ISSET(0, &reads)) // 키보드에서 입력 발생
                {

                        if((send_msg_len = read(0, send_msg, MSG_LEN)) > 0)
                        {
                                send_msg[send_msg_len] = '\0';

                                if(!strcmp(send_msg, "Q\n")) break;

                                if(write(clnt_sockfd, send_msg, send_msg_len) != send_msg_len)
                                        fputs("Message Sending Fail\n", stdout);
                        }
                }

                if(FD_ISSET(clnt_sockfd, &reads))       // 소켓에서 이벤트 발생
                {
                        if((recv_msg_len = read(clnt_sockfd, recv_msg, sizeof(recv_msg))) != 0)
                        {
                                recv_msg[recv_msg_len] = '\0';
                                fprintf(stdout, "Receive msg : %s\n", recv_msg);
                        }

                        if(!strcmp(recv_msg, "Q\n")) break;
                }
        }

        close(clnt_sockfd);

        return 0;
}

void
vf_error_handling(char* msg)
{
        fputs(msg, stderr);
        fputc('\n', stderr);
        exit(1);
}
