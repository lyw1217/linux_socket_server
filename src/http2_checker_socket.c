/*
 * title        : http/2 checker socket client
 * name         : LEE YOUNG WOO
 * date         : 20.03.27
 *
 * needs to
 *      create a socket
 *      lookup the IP address
 *      open the socket
 *      send the request
 *      wait for the response
 *      close the socket
 *
 * */

#include <stdio.h> /* fput... */
#include <stdlib.h> /* exit */
#include <string.h> /* memset, memcpy */
#include <unistd.h> /* read, write, close */
// #include <sys/socket.h> /* socket, connect.. */
#include <netinet/in.h> /*include <sys/socket.h>, sockaddr_in...*/
#include <netdb.h> /* struct hostent, gethostbyname*/

#define MSG_SIZE 1024
#define RSP_SIZE 4096

void vf_error_handling(char* msg);

int main(int argc, char *argv[])
{
        int portno;
        char *host;

        struct sockaddr_in serv_addr;   // AF_INET(IPv4)
        struct hostent *server;         // host에 대한 정보 구조체
        int sockfd;

        char message[MSG_SIZE], response[RSP_SIZE];
        int left_len, sent_len, recv_len, bytes;
        int timeout;

        char *token;
        int flag = 0;

        if(argc != 3)
        {
                fprintf(stderr, "Usage :  <port> <host>\n");
                exit(1);
        }

        // fill in the parameters
        portno = atoi(argv[1]);
        host = argv[2];
        sprintf(message, message_fmt, argv[2]);

        printf("port : %d, host : %s\nRequest : \n%s\n\n", portno, host, message);

        // create the socket
        sockfd = socket(PF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) vf_error_handling("socket() error");

        // lookup the IP address
        // gethonstbyname : 도메인 이름으로 hostent 정보를 구함
        server = gethostbyname(host);
        if(server == NULL) vf_error_handling("gethostbyname() error");

        // fill in the structure
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(portno);
        memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

        // connect the socket
        if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                vf_error_handling("connect() error");

        // send the request
        left_len = strlen(message);
        sent_len = 0;
        do{
                bytes = write(sockfd, message + sent_len, left_len - sent_len);
                if(bytes < 0)
                        vf_error_handling("write() error");
                if(bytes == 0)
                        break;
                sent_len += bytes;
        }while (sent_len < left_len);

        // receive the response
        memset(response, 0, sizeof(response));
        left_len = sizeof(response) - 1;
        recv_len = 0;
        timeout = 0;
        
        fprintf(stdout, "Response :\n");  
        do{
                memset(response, 0, sizeof(response));
                bytes = recv(sockfd, response, 1024, 0);

                token = strtok(response, " ");
                while(token != NULL)
                {
                        fprintf(stdout, "%s ", token);
                        if(!strcmp(token, "101"))
                        {
                                flag = 1;
                                break;
                        }
                        token = strtok(NULL, " ");
                }

                if(flag == 1)
                {
                        fputs("\nHTTP/2 is supported using h2c indicator\n", stdout);
                        break;
                }

                if(bytes < 0)
                        vf_error_handling("read() error");
                if(bytes == 0)
                        break;
                recv_len += bytes;
                if(timeout++ > 3){ fputs("\ntimeout\n", stderr); break; }
        }while(1);

        if(flag != 1)
                fputs("\nHTTP/2 is NOT supported using h2c indicator\n", stdout);

        if(recv_len == left_len)
                error("error");

        // close the socket
        close(sockfd);

        return 0;
}

void
vf_error_handling(char* msg)
{
        fputs(msg, stderr);
        fputc('\n', stderr);
        exit(1);
}
