/*
 * title        : rest socket client
 * name         : LEE YOUNG WOO
 * date         : 20.03.30
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
        char message_fmt[MSG_SIZE] = "GET %s HTTP/1.0\r\nAuthorization: %s\r\n\r\n";

        struct sockaddr_in serv_addr;   // AF_INET(IPv4)
        struct hostent *server;         // host에 대한 정보 구조체
        int sockfd;

        char message[MSG_SIZE], response[RSP_SIZE];
        int left_len, sent_len, recv_len, bytes;

        if(argc < 4)
        {
                fprintf(stderr, "Usage :  <port> <host> <resource> <API_KEYS>\n");
                fprintf(stderr, "ex) 80 maps.googleapis.com /maps/api/geocode/json?address=chicago <API_KEYS>\n");
                exit(1);
        }

        // fill in the parameters
        portno = atoi(argv[1]);
        host = argv[2];
        sprintf(message, message_fmt, argv[3], argv[4]);

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

        do{
                bytes = read(sockfd, response + recv_len, left_len - recv_len);
                if(bytes < 0)
                        vf_error_handling("read() error");
                if(bytes == 0)
                        break;
                recv_len += bytes;
        }while(1);

        if(recv_len == left_len)
                error("error");

        // close the socket
        close(sockfd);

        fprintf(stdout, "Response :\n%s\n", response);

        return 0;
}

void
vf_error_handling(char* msg)
{
        fputs(msg, stderr);
        fputc('\n', stderr);
        exit(1);
}
