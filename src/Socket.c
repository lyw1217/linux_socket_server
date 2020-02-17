/*
 ============================================================================
 Name        : Socket.c
 Author      : LYW
 Version     : 2.0.0
 Copyright   : Your copyright notice
 Description : o sign up : [ID]sign@PW
 	 	 	   o send all msg : [ID]msg@msg..
 	 	 	   o send chat : [ID]receiver ID@msg@msg..
 	 	 	   o receive chat : [sender ID]msg@msg@...
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h> // ������������ ��� ����
#include <time.h>

//#define DEBUG			1

#define BUF_SIZE 		256
#define EPOLL_SIZE 		50
#define MAX_CLIENT 		20
#define IP_ADDR_SIZE	20
#define MAX_USER_LEN	20
// �Լ� �⺻�� ����
void error_handling(char *buf);
void print_time();
void print_log(char *buff);
int write_logfile(char *buff);

// �������� ����
char log_buff[BUF_SIZE] = { 0x00, };	// �α׸� ���� ����
volatile time_t rawtime; // �ð� ����� ���� �������� ����
volatile struct tm tm;

// ���� ��� ����ü ����
struct member {
	char id[MAX_USER_LEN];
	char pw[MAX_USER_LEN];
	int fd;
};

int main(int argc, char *argv[]) {
	int server_sock, client_sock;	// �� ���� ��ȣ
	struct sockaddr_in server_addr, client_addr; // sockaddr_in : AF_INET(IPv4)���� ����ϴ� ����ü
	socklen_t addr_size;
	int str_len, i, j, k, l, num, log_Temp_len;
	char buf[BUF_SIZE];
	char split_buf[BUF_SIZE];
	char addr_Temp[IP_ADDR_SIZE];	// ip �ּ� ����� ���� Temp
	char log_Temp[256]; // log ������ ���� temp
	char *splitLists[10];	// split�� ���� ������ �迭

	char delimeter[4] = "[]@";
	char *token;
	int split_count = 0;

	int client_fd[MAX_CLIENT] = { 0, };
	int client_index = 0;
	int time_out = 0;

	// ���� ��� ����ü �迭 ����, �ʱ�ȭ
	struct member User[MAX_CLIENT] = { { NULL, NULL, 0 }, };

	int user_count = 0;
	int signup_flag = 0;
	
	int chat_flag = 0;
	int incorrect_flag = 0;

	// �ð� ����� ���� �������� �ʱ�ȭ
	rawtime = time(NULL);
	tm = *localtime(&rawtime);

	struct epoll_event *ep_events;
	struct epoll_event event;
	int epfd, event_cnt;

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	// ���� ����, int������ ���� ��ȣ�� �����Ǿ� ��ȯ
	if ((server_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		error_handling("Server : Can't open stream socket");

	memset(&server_addr, 0, sizeof(server_addr));	// server_Addr �� NULL�� �ʱ�ȭ
	server_addr.sin_family = AF_INET;				// IPv4 : AF_INET
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);// INADDR_ANY ����� ���� �ڽ��� IP���� �ڵ����� �Ҵ�
	server_addr.sin_port = htons(atoi(argv[1]));// main�Լ��� �Ķ���ͷ� ���� ���� argv[1]�� ��Ʈ�� ����

	// bind() ȣ��, �Ķ���� (���Ϲ�ȣ, ������ �����ּ� ����ü ������, ����ü�� ũ��) ������ 0, ���н� -1 ����
	if (bind(server_sock, (struct sockaddr*) &server_addr, sizeof(server_addr))
			== -1) {
		close(server_sock);
		error_handling("Server : bind() error");
	}

	// ������ ���� ������ ����, �Ķ����(���� ��ȣ, ���� ����ϴ� Ŭ���̾�Ʈ �ִ� ��)
	// Ŭ���̾�Ʈ�� listen()�� ȣ���� �� ���� ������ �������� connect()�� ȣ��
	// (���⼭ 3-way �ڵ彦��ũ�� �߻��Ѵ�. ����Ȯ��) , ������ 0, ���н� -1
	// �ý����� �ڵ彦��ũ�� ��ģ �Ŀ��� ���� ���α׷��� ������ ������ �޾Ƶ��̴� �������� accept()�� ����
	if (listen(server_sock, MAX_CLIENT) == -1) {
		close(server_sock);
		error_handling("Server : listen() error");

	}

	// Ŀ���� �����ϴ� epoll �ν��Ͻ��� �Ҹ��� ���� ��ũ������ ����� ����
	// ���� �� epoll ���� ��ũ����, ���н� -1 ��ȯ
	if ((epfd = epoll_create(EPOLL_SIZE)) == -1)
		error_handling("Server : epoll_create() error");

	if ((ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE)) == NULL) {
		close(server_sock);
		close(epfd);
		error_handling("Server : malloc() error");
	}

	event.events = EPOLLIN;
	event.data.fd = server_sock;
	// ���� ��ũ����(server_sock)�� epoll �ν��Ͻ��� ����Ѵ�. (��������� ���� �̺�Ʈ ������ EPOLLIN)
	epoll_ctl(epfd, EPOLL_CTL_ADD, server_sock, &event);

	/*�α�ó����ƾ*/
	memset(log_Temp, 0x00, sizeof(log_Temp)); // �����ʱ�ȭ
	sprintf(log_Temp, "Server Start : Waiting Connection Request.");
	print_log(log_Temp);

	/* while�� ���� */
	while (1) {
		// ���� �� �̺�Ʈ�� �߻��� ���� ��ũ������ ��, ���� �� -1 ��ȯ
		// �� ��° ���ڷ� ���޵� �ּ��� �޸� ������ �̺�Ʈ �߻��� ���� ��ũ���Ϳ� ���� ������ ����ִ�.
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
//		if (event_cnt == -1) {
			puts("Server : epoll_wait() error");
			break;
		}

		for (i = 0; i < event_cnt; i++) {
			if (ep_events[i].data.fd == server_sock) {
				addr_size = sizeof(client_addr);

				// ����� Ŭ���̾�Ʈ�� �����ּ� ����ü�� �����ּ� ����ü�� ���̸� ����
				client_sock = accept(server_sock,
						(struct sockaddr*) &client_addr, &addr_size);
				event.events = EPOLLIN;
				event.data.fd = client_sock;
				// ���� ��ũ����(client_sock)�� epoll �ν��Ͻ��� ����Ѵ�. (��������� ���� �̺�Ʈ ������ EPOLLIN)
				epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &event);

				// IPv4 �� IPv6 �ּҸ� binary ���¿��� ����� �˾ƺ��� ���� �ؽ�Ʈ(human-readable text)���·� ��ȯ���ش�.
				inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, addr_Temp,
						sizeof(addr_Temp));

				/*�α�ó����ƾ*/
				memset(log_Temp, 0x00, sizeof(log_Temp)); // �����ʱ�ȭ
				sprintf(log_Temp, "Server : Connect Request %d, IP : %s ",
						client_sock, addr_Temp);
				print_log(log_Temp);

				// Ŭ���̾�Ʈ ���ϵ�ũ���� ��ȣ ���� �� ����ó��
				time_out = 0;
				while (client_fd[client_index] != 0) {
					client_index++;
					if (client_index >= MAX_CLIENT)
						client_index = 0;
					time_out++;
					if (time_out >= (MAX_CLIENT * 2)) {
						error_handling(
								"Server : Client Index is Full Error!\n");
					}
				}
				client_fd[client_index] = client_sock;
			} else {
				// read�Լ��� BUF_SIZE��ŭ �о buf�� ����, ���� ���� ���̸� ��ȯ
				// ���� 10����Ʈ�� �Դٸ� BUF_SIZE��ŭ �е��� �ϱ� ������ ������ �� 10����Ʈ��ŭ�� ��ȯ
				memset(buf, 0x00, sizeof(buf));
				str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);

				if (str_len == 0) { // close request!
					epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
					close(ep_events[i].data.fd);

					/*�α�ó����ƾ*/
					memset(log_Temp, 0x00, sizeof(log_Temp)); // �����ʱ�ȭ
					sprintf(log_Temp, "Server : Closed client %d, IP : %s",
							ep_events[i].data.fd, addr_Temp);
					print_log(log_Temp);

					// Ŭ���̾�Ʈ ���ϵ�ũ���� ��ȣ �� ���� �ʱ�ȭ
					for (num = 0; num < MAX_CLIENT; num++) {
						if (client_fd[num] == ep_events[i].data.fd)
							client_fd[num] = 0;
						if(User[num].fd == ep_events[i].data.fd){
							User[num].fd = 0;
							User[num].id[0] = NULL;
							User[num].pw[0] = NULL;
						}
							
					}
				} else {
					/*�α�ó����ƾ*/
					memset(log_Temp, 0x00, sizeof(log_Temp)); // �����ʱ�ȭ
					sprintf(log_Temp, "Server : Client %d send : %s",
							ep_events[i].data.fd, buf);
					print_log(log_Temp);

					/*���ڿ� ó���� [split]*/
					memset(split_buf, 0x00, sizeof(split_buf));
					memset(splitLists, 0x00, sizeof(splitLists));
					strcpy(split_buf, buf);
					split_count = 0;
					token = strtok(split_buf, delimeter);
					while (token) {
						splitLists[split_count] = token;
						token = strtok(NULL, delimeter);
						split_count++;
					}

					// if received msg = [ALLMSG]abc@123
					// splitLists[0] : ALLMSG | splitLists[1] : abc | splitLists[2] : 123
					if (splitLists[0] != NULL) {
						if (splitLists[1] != NULL) {
							///////////////////////////////////////////////////////////
							//*********************** ���ڿ� �񱳺� **********************//
							///////////////////////////////////////////////////////////
							if (!strcmp(splitLists[1], "sign")) {
								// [ID]sign@PW �������� ����
								// ���� DB ����
								// �ߺ� ID Ȯ��
								for (num = 0; num < MAX_CLIENT; num++) {
									if (User[num].id != NULL) {
										// ����ó��
										if (!strcmp(splitLists[0],
												User[num].id)) {
											// �ߺ��� id�� �ִٸ�
											/*�α�ó����ƾ*/
											memset(log_Temp, 0x00,
													sizeof(log_Temp)); // �����ʱ�ȭ
											sprintf(log_Temp,
													"Server : %s is already exist",
													splitLists[0]);
											log_Temp_len = strlen(log_Temp);
											if (write(ep_events[i].data.fd,
													log_Temp, log_Temp_len)
													!= log_Temp_len) {
												error_handling(
														"write() Error!");
											}
#ifdef DEBUG
											printf("���̵� �ߺ� write\n");
#endif
											print_log(log_Temp);
											break;
										}
									}
									// ���������� �ߺ��� ID�� ���ٸ� �÷���
									if (num >= (MAX_CLIENT - 1)) {
										signup_flag = 1;
									}
								}
								if (signup_flag) {
									signup_flag = 0;
									// �ߺ��� id ������ ����ü�� ���
									// ���� pw�� �ȿ���? splitLists[2] �� ���ٸ� �����
									if(splitLists[2] == NULL){
										/*�α�ó����ƾ*/
										memset(log_Temp, 0x00,
												sizeof(log_Temp)); // �����ʱ�ȭ
										sprintf(log_Temp, "Server : Input Password Please. ex) [ID]sign@PW");
										log_Temp_len = strlen(log_Temp);
										if (write(ep_events[i].data.fd,	log_Temp, log_Temp_len)	!= log_Temp_len) {
											error_handling("write() Error!");
										}
										print_log(log_Temp);
#ifdef DEBUG
										printf("�н����� ���Է� write\n");
#endif
										break;
									}	
									for (num = 0; num < strlen(splitLists[0]);
											num++) {
										User[user_count].id[num] =
												splitLists[0][num];
									}
									User[user_count].id[++num] = NULL;
									for (num = 0; num < strlen(splitLists[2]);
											num++) {
										User[user_count].pw[num] =
												splitLists[2][num];
									}
									User[user_count].pw[++num] = NULL;
									User[user_count].fd = ep_events[i].data.fd;

									/*�α�ó����ƾ*/
									memset(log_Temp, 0x00, sizeof(log_Temp)); // �����ʱ�ȭ
									sprintf(log_Temp,
											"Server : Client '%s' sign up!",
											User[user_count].id);
									log_Temp_len = strlen(log_Temp);
									if (write(ep_events[i].data.fd, log_Temp,
											log_Temp_len) != log_Temp_len) {
										error_handling("write() Error!");
									}
									print_log(log_Temp);
#ifdef DEBUG
									printf("���̵� ���ԿϷ� write\n");
#endif
									user_count++;
									if (user_count >= MAX_CLIENT) {
										user_count = 0;
									}
								}
							} else {

								// sign������ �ƴ϶�� [ID]�� �����ϴ��� Ȯ��
								for (num = 0; num < MAX_CLIENT; num++) {
									if (User[num].id != NULL) {
										// ����ó��
										if (!strcmp(splitLists[0], User[num].id)) {
											// ù ��° splitLists�� ��ġ�ϴ� ID�� �ִ��� Ȯ���ؼ� �ִٸ�,
											// �� ��° splitLists�� �ܾ ���� ã�´�.
											if (!strcmp(splitLists[1], "Q")) {
												// �� ��° splitLists�� Q ��ɾ��̸� ���� ����
												/*�α�ó����ƾ*/
												memset(log_Temp, 0x00,
														sizeof(log_Temp)); // �����ʱ�ȭ
												sprintf(log_Temp,
														"Server : Close Server.");
												log_Temp_len = strlen(log_Temp);
												if (write(ep_events[i].data.fd,
														log_Temp, log_Temp_len)
														!= log_Temp_len) {
													error_handling(
															"write() Error!");
												}
												print_log(log_Temp);
												close(server_sock);
												close(epfd);
#ifdef DEBUG
												printf("���ϼ��� ���� write\n");
#endif
												return 0;
											} else if (!strcmp(splitLists[1], "IDLIST")) {
												// �� ��° splitLists�� �ܾ IDLIST ��ɾ��̸�
												// ���� ���ӵǾ� �ִ� ID ��ȸ
												
												/*�α�ó����ƾ*/
												memset(log_Temp, 0x00, sizeof(log_Temp)); // �����ʱ�ȭ
												for(num = 0; num < MAX_CLIENT; num++){
													if(User[num].id != NULL){
														// id�� ���� ���ڿ��� �߰�
														strcat(log_Temp, User[num].id);
														strcat(log_Temp, " ");
													}
												}
												log_Temp_len = strlen(log_Temp);
												print_log(log_Temp);
												if (write(ep_events[i].data.fd, log_Temp, log_Temp_len) != log_Temp_len) {
													error_handling("write() Error!");
												}
#ifdef DEBUG
												printf("IDLIST ��ȸ write\n");
#endif
											} else {
												// ��ɾ �ƴϰ�
												// splitLists[1] �� �Ⱑ�Ե� User.ID�� �ִٸ�
												// [��������ID]split1@split2@... ������ ����
												for(j = 0; j < MAX_CLIENT; j++){
													if(User[j].id != NULL){
														if(!strcmp(splitLists[1], User[j].id)){
															chat_flag = 1;

															memset(log_Temp, 0x00,
																	sizeof(log_Temp)); // �����ʱ�ȭ
															// ������ ����� [ID] �߰�
															strcat(log_Temp, "[");

															for(k = 0; k < MAX_CLIENT; k++){
																// ���� ���ڿ��� ������ ����� ���̵� �߰�
																if(User[k].fd == ep_events[i].data.fd){
																	for(l = 0; l < strlen(User[k].id); l++){
																		log_Temp[l+1] = User[k].id[l];
																	}
																	break;
																}
																if((k >= (MAX_CLIENT-1)) && (User[k].fd != ep_events[i].data.fd)){
																	// ���� ���� ��ũ���Ϳ� �α��� �� ������ �ٸ��� �޽����� �������� �ϴ� ���

																	/*�α�ó����ƾ*/
																	memset(log_Temp, 0x00, sizeof(log_Temp)); // �����ʱ�ȭ
																	sprintf(log_Temp, "Server : Sign in with correct ID, Closed client %d, IP : %s", ep_events[i].data.fd, addr_Temp);
																	print_log(log_Temp);
																	incorrect_flag = 1;
																	break;
																}
															}
															if(incorrect_flag == 1){
																// �߸��� ID�� �޽��� ������ ��� �������� �ʰ� break;
																incorrect_flag = 0;
																break;
															}

															strcat(log_Temp, "]");
															// �������� �ϴ� �޽��� �߰�
															k = 2;
															while(splitLists[k] != NULL){
																strcat(log_Temp, splitLists[k]);
																strcat(log_Temp, "@");
																k++;
															}
															log_Temp_len = strlen(log_Temp);
															// �ش� User.ID�� ���ϵ�ũ���� ��ȣ�� ������.
															if (write(User[j].fd, log_Temp, log_Temp_len) != log_Temp_len) {
																error_handling("write() Error!");
															}
															/*�α�ó����ƾ*/
															memset(buf, 0x00, sizeof(buf));
															sprintf(buf, "[Client %d] send '%s' to [Client %d]", User[num].fd, log_Temp, User[j].fd);
															print_log(buf);
#ifdef DEBUG
															printf("�ش� �Ѱ� ���̵�� write\n");
#endif
															break;
														}
													}
												}
												if(chat_flag){
													chat_flag = 0;
												}else{
													// splitLists[1]�� �ƹ� ���ڵ� ���ٸ�, �ùٸ� ��ɾ ���ٸ� ALLMSG
													/*�α�ó����ƾ*/
													memset(log_Temp, 0x00,
															sizeof(log_Temp)); // �����ʱ�ȭ
													sprintf(log_Temp, "%s", buf);
													log_Temp_len = strlen(log_Temp);
													for (num = 0; num < MAX_CLIENT;	num++) {
														if(User[num].fd != 0){
															if (write(User[num].fd, log_Temp,	log_Temp_len) != log_Temp_len) {
																error_handling("write() Error!");
															}
#ifdef DEBUG
															printf("ALLMSG ���� write\n");
#endif
														}
													}
													/*�α�ó����ƾ*/
													memset(buf, 0x00, sizeof(buf));
													sprintf(buf, "[Client %d] send '%s' to All Users", User[num].fd, log_Temp);
													print_log(buf);
												}
											}
											///////////////////////////////////
											////////////���ڿ� �񱳺� ��/////////////
											///////////////////////////////////
											break;
										}
									}

									if (num == (MAX_CLIENT - 1)) {
										// ��ϵ� id�� ���ٸ� fail msg �۽�, ��� �� break;
										/*�α�ó����ƾ*/
										memset(log_Temp, 0x00,
												sizeof(log_Temp)); // �����ʱ�ȭ
										sprintf(log_Temp,
												"Incorrect ID, Try again");
										print_log(log_Temp);
										log_Temp_len = strlen(log_Temp);
										if (write(ep_events[i].data.fd,
												log_Temp, log_Temp_len)
												!= log_Temp_len) {
											error_handling("write() Error!");
										}
#ifdef DEBUG
										printf("��ϵ� id ���� write\n");
#endif
									}
								}
							}
						} else {
							//splitLists[1] == NULL �̶��
							/*�α�ó����ƾ*/
							memset(log_Temp, 0x00, sizeof(log_Temp)); // �����ʱ�ȭ
							sprintf(log_Temp, "Incorrect Message, Try again");
							print_log(log_Temp);
							log_Temp_len = strlen(log_Temp);
							if (write(ep_events[i].data.fd, log_Temp,
									log_Temp_len) != log_Temp_len) {
								error_handling("write() Error!");
							}
#ifdef DEBUG
							printf("�޽��� �Ⱥ��� write\n");
#endif
						}

					}
				}
			}
		}
	}
	/* while�� �� */

	close(server_sock);
	close(epfd);
	return 0;
}

void error_handling(char *buf) {
	fputs(buf, stderr);
	fputc('\n', stderr);

	exit(1);
}

void print_time() {
	// �������� ó��
	rawtime = time(NULL);
	tm = *localtime(&rawtime);

	printf("%02d-%02d-%02d %02d:%02d:%02d ", tm.tm_year - 100, tm.tm_mon + 1,
			tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void print_log(char *buff) {
	// �α� ó�� ��ƾ
	print_time();
	printf(buff);
	printf("\n");

	memset(log_buff, 0x00, sizeof(log_buff)); // �����ʱ�ȭ
	sprintf(log_buff, "%02d-%02d-%02d %02d:%02d:%02d %s\n", tm.tm_year - 100,
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, buff); // �޼��� ����
	write_logfile(log_buff); //�����Լ� ȣ��
}

// �α� ���� �Լ�
int write_logfile(char *buff) {
	int file_access;

	FILE *fd_log_file;

	char log_file[32] = { 0, };
	sprintf(log_file, "/project/Socket/%02d_%02d_%02d_socket.log", tm.tm_year - 100,
			tm.tm_mon + 1, tm.tm_mday);

	file_access = access(log_file, R_OK | W_OK);

	if (file_access == 0) {
		fd_log_file = fopen(log_file, "a+");
	} else {
		fd_log_file = fopen(log_file, "w+");
	}

	if (fd_log_file == NULL) {
		printf("log file open error\n");
		return -1;
	}

	fseek(fd_log_file, 0, SEEK_END);

	fwrite(buff, 1, strlen(buff), fd_log_file);
	fclose(fd_log_file);

	return 1;
}
