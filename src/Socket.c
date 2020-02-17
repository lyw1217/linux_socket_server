/*
 ============================================================================
 Name        : Socket.c
 Author      : LYW
 Version     : 1.1.0
 Copyright   : Your copyright notice
 Description : Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ���� ���α׷��ֿ� ���� ������� ����

#define BUF_LEN 128
// �޽��� �ۼ��ſ� ���� ���� ũ�⸦ ����
#define MAX_CLIENT 5
// ���� ���� ����ϴ� �ִ� Ŭ���̾�Ʈ �� ����

int main(int argc, char *argv[])
{
    char buffer[BUF_LEN];
    struct sockaddr_in server_addr, client_addr;	// sockaddr_in : AF_INET(IPv4)���� ����ϴ� ����ü
    char temp[20];
    int server_fd, client_fd;
    //server_fd, client_fd : �� ���� ��ȣ
    int len, msg_size;

    // ���� �� �� ���� �Ķ���Ͱ� ������
    if(argc != 2)
    {
        printf("usage : %s [port]\n", argv[0]);
        exit(0);
    }

    // ���� ����, int������ ���� ��ȣ�� �����Ǿ� ��ȯ
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Server : Can't open stream socket\n");
        exit(0);
    }
    memset(&server_addr, 0x00, sizeof(server_addr));	// server_Addr �� NULL�� �ʱ�ȭ
    server_addr.sin_family = AF_INET;					// IPv4 : AF_INET
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 	// INADDR_ANY ����� ���� �ڽ��� IP���� �ڵ����� �Ҵ�
    server_addr.sin_port = htons(atoi(argv[1]));		// main�Լ��� �Ķ���ͷ� ���� ���� argv[1]�� ��Ʈ�� ����
    // server_addr ���� // ������ ������ �ּ� ����

    // bind() ȣ��, �Ķ���� (���Ϲ�ȣ, ������ �����ּ� ����ü ������, ����ü�� ũ��) ������ 0, ���н� -1 ����
    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <0)
    {
        printf("Server : Can't bind local address.\n");
        exit(0);
    }

    // ������ ���� ������ ����, �Ķ����(���� ��ȣ, ���� ����ϴ� Ŭ���̾�Ʈ �ִ� ��)
    // Ŭ���̾�Ʈ�� listen()�� ȣ���� �� ���� ������ �������� connect()�� ȣ��
    // (���⼭ 3-way �ڵ彦��ũ�� �߻��Ѵ�. ����Ȯ��) , ������ 0, ���н� -1
    // �ý����� �ڵ彦��ũ�� ��ģ �Ŀ��� ���� ���α׷��� ������ ������ �޾Ƶ��̴� �������� accept()�� ����
    if(listen(server_fd, MAX_CLIENT) < 0)
    {
        printf("Server : Can't listening connect.\n");
        exit(0);
    }

    memset(buffer, 0x00, sizeof(buffer));	// ���� �ʱ�ȭ
    printf("Server : Wating connection request.\n");
    len = sizeof(client_addr);

    /* while�� ���� */
    while(1)
    {
    	// ����� Ŭ���̾�Ʈ�� �����ּ� ����ü�� �����ּ� ����ü�� ���̸� ����
    	// accept()�� �ѹ��� �ϳ��� ���Ḹ ����.
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if(client_fd < 0)
        {
            printf("Server: accept failed.\n");
            exit(0);
        }

        //IPv4 �� IPv6 �ּҸ� binary ���¿��� ����� �˾ƺ��� ���� �ؽ�Ʈ(human-readable text)���·� ��ȯ���ش�.
        inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, temp, sizeof(temp));

        printf("Server : %s client connected.\n", temp);

        // read�Լ��� 1024����Ʈ��ŭ �о buffer�� ����, ���� ���� ���̸� ��ȯ
        // ���� 10����Ʈ�� �Դٸ� 1024����Ʈ��ŭ �е��� �ϱ� ������ ������ �� 10����Ʈ��ŭ�� ��ȯ
        msg_size = read(client_fd, buffer, 1024);

        // Ŭ���̾�Ʈ���� msg_size��ŭ ������ buffer ������ write
        write(client_fd, buffer, msg_size);

        close(client_fd);
        printf("Server : %s client closed.\n", temp);
    }
    /* while�� �� */

    close(server_fd);
    return 0;
}
