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

#define MSG_LEN 30

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

        while(1)
        {
                fputs("Input Message(Q to quit) : ", stdout);
                if(send_msg_len = read(0, send_msg, MSG_LEN) > 0)
                {
                        send_msg[send_msg_len] = '\0';

                        if(!strcmp(send_msg, "Q\n")) break;
                        fprintf(stdout, "Send Data\n");

                        if(send_msg_len = write(clnt_sockfd, send_msg, strlen(send_msg)) < 0)
                                vf_error_handling("write() error");

                        left_msg_len = send_msg_len;
                        recv_msg_len = 0;

                        while(left_msg_len > 0)
                        {
                                if((send_msg_len = read(clnt_sockfd, &recv_msg[recv_msg_len], left_msg_len)) < 0)
                                        vf_error_handling("read() error");

                                left_msg_len -= send_msg_len;
                                recv_msg_len += send_msg_len;
                        }

                        recv_msg[recv_msg_len] = '\0';
                        fprintf(stdout, "Recv Data : %s", recv_msg);
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
