/*
 * title        : linux socket server
 * name         : LEE YOUNG WOO
 * date         : 20.03.25
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

        int serv_sockfd, clnt_sockfd;
        struct sockaddr_in serv_addr, clnt_addr;
        struct timeval timeout;

        fd_set reads, cpy_reads; // 파일 상태 테이블 선언

        socklen_t addr_sz;
        int fd_max, msg_len, fd_num, i;

        char msg[MSG_LEN];

        if((serv_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
                vf_error_handling("socket() error");

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(atoi(argv[1]));

        if(bind(serv_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
                vf_error_handling("bind() error");

        if(listen(serv_sockfd, 5) == -1)
                vf_error_handling("listen() error");

        FD_ZERO(&reads);                // fd_set 테이블 초기화
        FD_SET(serv_sockfd, &reads);    // 서버 소켓의 이벤트 검사를 위해 fs_set 테이블에 추가
        fd_max = serv_sockfd;

        while(1)
        {
                cpy_reads = reads;
                timeout.tv_sec = 5;
                timeout.tv_usec = 500000;       // timeout : 5.5sec

                /* result
                * -1 : select 함수 오류
                *  0 : timeout 동안 아무런 사건 발생하지 않음
                * >0 : 커널로부터 반환받는 사건이 발생한 소켓 기술자 수
                */
                if((fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == -1)
                        break;

                if(fd_num == 0)
                        continue;

                for(i=0; i < fd_max + 1; i++){
                        if(FD_ISSET(i, &cpy_reads))     // fd_set 중 소켓 fd에 해당하는 비트가 set 되어있으면 양수인 fd 반환
                        {
                                if(i == serv_sockfd)    // serv_sockfd에 이벤트(연결 요청) 발생
                                {
                                        addr_sz = sizeof(clnt_addr);
                                        clnt_sockfd = accept(serv_sockfd, (struct sockaddr*)&clnt_addr, &addr_sz);
                                        FD_SET(clnt_sockfd, &reads);    // fd_set 테이블에 clnt_sockfd를 추가
                                        if(fd_max < clnt_sockfd)
                                                fd_max = clnt_sockfd;
                                        fprintf(stdout, "connected client : %d\n", clnt_sockfd);
                                }
                                else                    // 클라이언트와 연결된 소켓에 이벤트 발생
                                {
                                        msg_len = read(i, msg, MSG_LEN);
                                        if(msg_len == 0)
                                        {
                                                FD_CLR(i, &reads);      // fd_set 테이블에서 파일 디스크립터를 삭제
                                                close(i);
                                                fprintf(stdout, "closed client : %d\n", i);
                                        }
                                        else
                                        {
                                                write(i, msg, msg_len); // echo
                                        }
                                }
                        }
                }
        }

        close(serv_sockfd);

        return 0;
}

void
vf_error_handling(char* msg)
{
        fputs(msg, stderr);
        fputc('\n', stderr);
        exit(1);
}
