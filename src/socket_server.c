/*
  2  * title        : linux socket server
  3  * name         : LEE YOUNG WOO
  4  * date         : 20.03.25
  5  *
  6  * */
  7 
  8 #include <stdio.h>
  9 #include <stdlib.h>
 10 #include <string.h>
 11 
 12 #include <arpa/inet.h>
 13 #include <unistd.h>
 14 #include <sys/socket.h>
 15 #include <sys/types.h>
 16 
  1 /*
  2  * title        : linux socket server
  3  * name         : LEE YOUNG WOO
  4  * date         : 20.03.25
  5  *
  6  * */
  7 
  8 #include <stdio.h>
  9 #include <stdlib.h>
 10 #include <string.h>
 11 
 12 #include <arpa/inet.h>
 13 #include <unistd.h>
 14 #include <sys/socket.h>
 15 #include <sys/types.h>
 16 
 17 #define MSG_LEN 30
 18 
 19 void vf_error_handling(char * msg);
 20 
 21 int main(int argc, char *argv[])
 22 {
 23         if(argc != 2)
 24         {
 25                 fprintf(stderr, "Usage : %s <port>\n", argv[0]);
 26                 exit(1);
 27         }
 28 
 29         int serv_sockfd, clnt_sockfd;
 30         struct sockaddr_in serv_addr, clnt_addr;
 31         struct timeval timeout;
 32 
 33         fd_set reads, cpy_reads; // 파일 상태 테이블 선언
 34 
 35         socklen_t addr_sz;
 36         int fd_max, msg_len, fd_num, i;
 37 
 38         char msg[MSG_LEN];
 39 
 40         if((serv_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
 41                 vf_error_handling("socket() error");
 42 
 43         memset(&serv_addr, 0, sizeof(serv_addr));
 44         serv_addr.sin_family = AF_INET;
 45         serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
 46         serv_addr.sin_port = htons(atoi(argv[1]));
 47 
 48         if(bind(serv_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
 49                 vf_error_handling("bind() error");
 50 
 51         if(listen(serv_sockfd, 5) == -1)
 52                 vf_error_handling("listen() error");
 53 
 54         FD_ZERO(&reads);                // fd_set 테이블 초기화
 55         FD_SET(serv_sockfd, &reads);    // 서버 소켓의 이벤트 검사를 위해 fs_set 테이블에 추가
 56         fd_max = serv_sockfd;
 57 
 58         while(1)
 59         {
 60                 cpy_reads = reads;
 61                 timeout.tv_sec = 5;
 19 void vf_error_handling(char * msg);
 20 
 21 int main(int argc, char *argv[])
 22 {
 23         if(argc != 2)
 24         {
 25                 fprintf(stderr, "Usage : %s <port>\n", argv[0]);
 26                 exit(1);
 27         }
 28 
 29         int serv_sockfd, clnt_sockfd;
 30         struct sockaddr_in serv_addr, clnt_addr;
 31         struct timeval timeout;
 32 
 33         fd_set reads, cpy_reads; // 파일 상태 테이블 선언
 34 
 35         socklen_t addr_sz;
 36         int fd_max, msg_len, fd_num, i;
 37 
 38         char msg[MSG_LEN];
 39 
 40         if((serv_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
 41                 vf_error_handling("socket() error");
 42 
 43         memset(&serv_addr, 0, sizeof(serv_addr));
 44         serv_addr.sin_family = AF_INET;
 45         serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
 46         serv_addr.sin_port = htons(atoi(argv[1]));
 47 
 48         if(bind(serv_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
 49                 vf_error_handling("bind() error");
 50 
 51         if(listen(serv_sockfd, 5) == -1)
 52                 vf_error_handling("listen() error");
 53 
 54         FD_ZERO(&reads);                // fd_set 테이블 초기화
 55         FD_SET(serv_sockfd, &reads);    // 서버 소켓의 이벤트 검사를 위해 fs_set 테이블에 추가
 56         fd_max = serv_sockfd;
 57 
 58         while(1)
 59         {
 60                 cpy_reads = reads;
 61                 timeout.tv_sec = 5;
 62                 timeout.tv_usec = 500000;       // timeout : 5.5sec
 63 
 64                 /* result
 65                 * -1 : select 함수 오류
 66                 *  0 : timeout 동안 아무런 사건 발생하지 않음
 67                 * >0 : 커널로부터 반환받는 사건이 발생한 소켓 기술자 수
 68                 */
 69                 if((fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == -1)
 70                         break;
 71 
 72                 if(fd_num == 0)
 73                         continue;
 74 
 75                 for(i=0; i < fd_max + 1; i++){
 76                         if(FD_ISSET(i, &cpy_reads))     // fd_set 중 소켓 fd에 해당하는 비트가 set 되어있으면 양수인 fd 반환
 77                         {
 78                                 if(i == serv_sockfd)    // serv_sockfd에 이벤트(연결 요청) 발생
 79                                 {
 80                                         addr_sz = sizeof(clnt_addr);
 81                                         clnt_sockfd = accept(serv_sockfd, (struct sockaddr*)&clnt_addr, &addr_sz);
 82                                         FD_SET(clnt_sockfd, &reads);    // fd_set 테이블에 clnt_sockfd를 추가
 83                                         if(fd_max < clnt_sockfd)
 84                                                 fd_max = clnt_sockfd;
 85                                         fprintf(stdout, "connected client : %d\n", clnt_sockfd);
 86                                 }
 87                                 else                    // 클라이언트와 연결된 소켓에 이벤트 발생
 88                                 {
 89                                         msg_len = read(i, msg, MSG_LEN);
 90                                         if(msg_len == 0)
 91                                         {
 92                                                 FD_CLR(i, &reads);      // fd_set 테이블에서 파일 디스크립터를 삭제
 93                                                 close(i);
 94                                                 fprintf(stdout, "closed client : %d\n", i);
 95                                         }
 96                                         else
 97                                         {
 98                                                 write(i, msg, msg_len); // echo
 99                                         }
100                                 }
101                         }
102                 }
103         }
104 
105         close(serv_sockfd);
106 
107         return 0;
108 }
109 
110 void
111 vf_error_handling(char* msg)
112 {
113         fputs(msg, stderr);
114         fputc('\n', stderr);
115         exit(1);
116 }
