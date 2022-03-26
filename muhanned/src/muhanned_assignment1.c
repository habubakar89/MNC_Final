/**
 * @muhanned_assignment1
 * @author  Muhanned Ibrahim <muhanned@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "../include/global.h"
#include "../include/logger.h"
#include <string.h>

#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 100
#define BUFFER_SIZE 256
#define MSG_SIZE 256
#define MAXDATASIZE 256
#define NI_MAXHOST 1025
#define NI_MAXSERV 32

struct host {
	char* hostname;
	char* ip_addr;
	char* port_num;
	int num_msg_sent;
	int num_msg_rcv;
	//char* status;
	int fd;
	//struct host * blocked;
	//struct host * next_host;
	bool is_logged_in;
	bool is_server;
	bool is_initialized;
	//struct message * queued_messages;
};
struct message {
	char text[MAXDATASIZE];
	struct host * from_client;
	struct message * next_message;
	bool is_broadcast;
};

struct client {
	char* neighbor_hostname;
	char* neighbor_ip_addr;
	char* neighbor_port_num;
	int neighbor_fd;
};

int connect_to_host(char *server_ip, char *server_port);

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */

int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);
	/*Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));
	/*Start Here*/
	if(argc != 3) {
		exit(-1);
	}
	
	
	char your_ubit_name[] = "muhanned";
	//-----------------------------------------------------------------------------------------------------------------------	
	//Referenced: https://gist.github.com/listnukira/4045436
	int fdsocket;
	struct addrinfo hints, *res;
	char *google_dns_ip = "8.8.8.8";
	char *google_dns_port = "53";
	char *host_ip[16];
	char *host_port = argv[2];
	struct sockaddr_in my_addr;

	/* Set up hints structure */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	/* Fill up address structures */
	if (getaddrinfo(google_dns_ip, google_dns_port, &hints, &res) != 0)
		perror("getaddrinfo failed");

	/* Socket */
	fdsocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fdsocket < 0)
		perror("Failed to create socket");

	/* Connect */
	if (connect(fdsocket, res->ai_addr, res->ai_addrlen) < 0)
		perror("Connect failed");

	bzero(&my_addr, sizeof(my_addr));
	socklen_t len = sizeof(my_addr);
	getsockname(fdsocket, (struct sockaddr *) &my_addr, &len);
	inet_ntop(AF_INET, &my_addr.sin_addr, &host_ip, sizeof(host_ip));
	close(fdsocket);
	//printf("%s\n", host_ip);
	freeaddrinfo(res);
	//------------------------------------------------------------------------------------------------------------------------
	struct host clients[BACKLOG - 1];

	for (int i = 0; i < BACKLOG - 1; i++) {
		char *hostname = (char*) malloc(sizeof(char)*NI_MAXHOST);
                memset(hostname, '\0', NI_MAXHOST);
		clients[i].hostname = hostname;

		char *ip_addr = (char*) malloc(sizeof(char)*NI_MAXHOST);
		memset(ip_addr, '\0', NI_MAXHOST);
		clients[i].ip_addr = ip_addr;

		char *port_num = (char*) malloc(sizeof(char)*MAXDATASIZE);
		memset(port_num, '0', MAXDATASIZE);
		clients[i].port_num = port_num;

		clients[i].num_msg_sent = 0;
		clients[i].num_msg_rcv = 0;
		clients[i].fd = 0;
		clients[i].is_logged_in = false;
		clients[i].is_server = false;
		clients[i].is_initialized = false;
	}
	
	if (*argv[1] == 's') {
		int server_socket, head_socket, selret, sock_index, fdaccept=0, caddr_len;
		struct sockaddr_in client_addr;
		struct addrinfo hints, *res;
		fd_set master_list, watch_list;

		/* Set up hints structure */
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		/* Fill up address structures */
		if (getaddrinfo(NULL, argv[2], &hints, &res) != 0)
			perror("getaddrinfo failed");

		/* Socket */
		server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if(server_socket < 0)
			perror("Cannot create socket");

		/* Bind */
		if(bind(server_socket, res->ai_addr, res->ai_addrlen) < 0 )
			perror("Bind failed");

		freeaddrinfo(res);

		/* Listen */
		if(listen(server_socket, BACKLOG) < 0)
			perror("Unable to listen on port");

		/* ---------------------------------------------------------------------------- */

		
		/* Zero select FD sets */
		FD_ZERO(&master_list);
		FD_ZERO(&watch_list);

		/* Register the listening socket */
		FD_SET(server_socket, &master_list);
		/* Register STDIN */
		FD_SET(STDIN, &master_list);

		head_socket = server_socket;

		while(TRUE){
			memcpy(&watch_list, &master_list, sizeof(master_list));

			printf("\n[PA1-Server@CSE489/589]$ ");
			fflush(stdout);

			/* select() system call. This will BLOCK */
			selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
			if(selret < 0)
				perror("select failed.");

			/* Check if we have sockets/STDIN to process */
			if(selret > 0){
				/* Loop through socket descriptors to check which ones are ready */
				for(sock_index=0; sock_index<=head_socket; sock_index+=1){

					if(FD_ISSET(sock_index, &watch_list)){

						/* Check if new command on STDIN */
						if (sock_index == STDIN){
							char *cmd = (char*) malloc(sizeof(char)*CMD_SIZE);

							memset(cmd, '\0', CMD_SIZE);
							if(fgets(cmd, CMD_SIZE-1, stdin) == NULL) //Mind the newline character that will be written to cmd
								exit(-1);

							//printf("\nI got: %s\n", cmd);

							//Process PA1 commands here ...
							char author[] = "AUTHOR\n";
                        				if (strcmp(cmd, author) == 0) {
                                				cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
                                				cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", your_ubit_name);
                                				cse4589_print_and_log("[AUTHOR:END]\n");
                        				}

							char ip[] = "IP\n";
                        				if (strcmp(cmd, ip) == 0) {
                                				cse4589_print_and_log("[IP:SUCCESS]\n");
                                				cse4589_print_and_log("IP:%s\n", host_ip);
                                				cse4589_print_and_log("[IP:END]\n");
                        				}

							char port[] = "PORT\n";
                        				if (strcmp(cmd, port) == 0) {
                                				cse4589_print_and_log("[PORT:SUCCESS]\n");
                                				cse4589_print_and_log("PORT:%s\n", host_port);
                                				cse4589_print_and_log("[PORT:END]\n");
                        				}
							free(cmd);
						}
						/* Check if new client is requesting connection */
						else if(sock_index == server_socket){
							caddr_len = sizeof(client_addr);
							fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, &caddr_len);
							if(fdaccept < 0)
								perror("Accept failed.");

							printf("\nRemote Host connected!\n");
							//char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
							char *client_hostname = (char*) malloc(sizeof(char)*NI_MAXHOST);
                					memset(client_hostname, '\0', NI_MAXHOST);
                                                        if (getnameinfo((struct sockaddr *) &client_addr, caddr_len, client_hostname, sizeof(char)*NI_MAXHOST, NULL, 0, 0) == 0) {
								printf("client hostname = %s\n", client_hostname);
							}
							
							for (int i = 0; i < BACKLOG - 1; i++) {
								if (!(clients[i].is_initialized)) {
									clients[i].hostname = client_hostname;
									clients[i].fd = fdaccept;
									//printf("client fd: %d\n", fdaccept);
									clients[i].is_logged_in = true;
									clients[i].is_server = false;
									clients[i].is_initialized = false;
									break;
									//end loop
								}
							}
							//char welcome[] = "welcome to the server";
							//send(fdaccept, welcome, 21, 0);
							/* Add to watched socket list */
							FD_SET(fdaccept, &master_list);
							if(fdaccept > head_socket) head_socket = fdaccept;
							char *keep_adding = (char*) malloc(sizeof(char)*MAXDATASIZE);
							memset(keep_adding,'\0',MAXDATASIZE);
						        send(fdaccept,"One message is just sent for now ",4,0);
						/*	for(int i=0;i<BACKLOG-1;i++){
								if(clients[i].is_initialized && clients[i].fd != fdaccept){
									
								}
							}	/*/	
							/*	for (int i = 0; i < BACKLOG - 1; i++) {
								//if(clients[i].is_initialized && clients[i].fd == fdaccept) send(fdaccept,"123",3,0);	
								 	if (clients[i].is_initialized && clients[i].fd != fdaccept) {
									char *curr_client = (char*) malloc(sizeof(char)*(MAXDATASIZE));
									memset(curr_client, '\0', MAXDATASIZE);
									char space1[] = " ";
									char space2[] = " ";
									char space3[] = " ";
									char *curr_client_ip_addr = (char*) malloc(sizeof(char)*MAXDATASIZE);
									memset(curr_client_ip_addr, '\0', MAXDATASIZE);
									strcpy(curr_client_ip_addr, clients[i].ip_addr);
									//printf("%s\n", curr_client_ip_addr);
									char *curr_client_port_num = (char*) malloc(sizeof(char)*MAXDATASIZE);
									memset(curr_client_port_num, '\0', MAXDATASIZE);
									strcpy(curr_client_port_num, clients[i].port_num);
									//printf("%s\n", curr_client_port_num);
									char *curr_client_fd = (char*) malloc(sizeof(char)*MAXDATASIZE);
									memset(curr_client_fd, '\0', MAXDATASIZE);
									curr_client_fd = clients[i].fd;
									//printf("%d\n", curr_client_fd);
									char *curr_client_fd_string = (char*) malloc(sizeof(char)*(MAXDATASIZE));
									memset(curr_client_fd_string, '\0', MAXDATASIZE);
									sprintf(curr_client_fd_string, "%d", curr_client_fd);
									//printf("curr_client_fd_string: %s\n", curr_client_fd_string);
									strcat(space1, curr_client_fd_string);
									//printf("space1: %s\n", space1);
									strcat(curr_client_port_num, space1);
									//printf("curr_client_port_num: %s\n", curr_client_port_num);
									strcat(space2, curr_client_port_num);
									//printf("space2: %s\n", space2);
									strcat(curr_client_ip_addr, space2);
									//printf("curr_client_ip_addr: %s\n", curr_client_ip_addr);
									strcat(space3, curr_client_ip_addr);
									//printf("space3: %s\n", space3);
									char curr_client_info_string[] = "client_info";
									char *curr_client_info = (char*) malloc(sizeof(char)*MAXDATASIZE);
									memset(curr_client_info, '\0', MAXDATASIZE);
									strcpy(curr_client_info, curr_client_info_string);
									strcat(curr_client_info, space3);
									strcat(keep_adding,curr_client_info);
									//printf("curr_client_info: %s\n", curr_client_info);
									if (send(fdaccept, curr_client_info, strlen(curr_client_info), 0) == strlen(curr_client_info)) {
										printf("client info successfully sent\n");
									}
									else {
										printf("client info did not properly send\n");
									}
								}
							}*/
					//	send(fdaccept,keep_adding,strlen(keep_adding),0);
							//send(server, host_ip, strlen(host_ip), 0);
					
}
				/* Read from existing clients */
						else{
							/* Initialize buffer to receieve response */
							char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
							memset(buffer, '\0', BUFFER_SIZE);

							if(recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0){
								close(sock_index);
								printf("Remote Host terminated connection!\n");

								/* Remove from watched list */
								FD_CLR(sock_index, &master_list);
							}
							else {
								//Process incoming data from existing clients here ...

								//printf("\nClient sent me: %s\n", buffer);
								for (int i = 0; i < BACKLOG - 1; i++) {
									if(clients[i].fd == fdaccept && (!clients[i].is_initialized)) {
										char *client_ip = (char*) malloc(sizeof(char)*MAXDATASIZE);
										memset(client_ip, '\0', MAXDATASIZE);	
										char *client_port = (char*) malloc(sizeof(char)*MAXDATASIZE);
										memset(client_port, '\0', MAXDATASIZE);
										
										char delim[] = " ";
										client_ip = strtok(buffer, delim);
										char *client_ip_copy = (char*) malloc(sizeof(char)*MAXDATASIZE);
										memset(client_ip_copy, '\0', MAXDATASIZE);
										strcpy(client_ip_copy, client_ip);
										clients[i].ip_addr = client_ip_copy;
										printf("received client ip: %s\n", client_ip_copy);
										client_ip = strtok(NULL, delim);
										client_port = client_ip;
										char *client_port_copy = (char*) malloc(sizeof(char)*MAXDATASIZE);
										strcpy(client_port_copy, client_port);
										clients[i].port_num = client_port_copy;
										clients[i].is_initialized = true;
										printf("received client port: %s\n", client_port_copy);
										break;
									}
								}

								
								for (int i = 0; i < BACKLOG - 1; i++) {
									if (clients[i].is_initialized) {
										printf("\n------------------\n");
										printf("client: %d\n", i);
										printf("hostname: %s\n", clients[i].hostname);
										printf("ip_addr: %s\n", clients[i].ip_addr);
										printf("port_num: %s\n", clients[i].port_num);
										printf("------------------\n");	
									}
								}

								//printf("ECHOing it back to the remote host ... ");
								//if(send(fdaccept, buffer, strlen(buffer), 0) == strlen(buffer))
								//	printf("Done!\n");
								fflush(stdout);
							}

							free(buffer);
						}
					}
				}
			}
		}

		return 0;
		}
	else if (*argv[1] == 'c') {
		int server;



		
		struct client neighbors[BACKLOG - 2];

		for (int i = 0; i < BACKLOG - 2; i++) {
			char *neighbor_hostname = (char*) malloc(sizeof(char)*NI_MAXHOST);
			memset(neighbor_hostname, '\0', NI_MAXHOST);
			neighbors[i].neighbor_hostname = neighbor_hostname;

			char *neighbor_ip_addr = (char*) malloc(sizeof(char)*NI_MAXHOST);
			memset(neighbor_ip_addr, '\0', NI_MAXHOST);
			neighbors[i].neighbor_ip_addr = neighbor_ip_addr;

			char *neighbor_port_num = (char*) malloc(sizeof(char)*MAXDATASIZE);
			memset(neighbor_port_num, '0', MAXDATASIZE);
			neighbors[i].neighbor_port_num = neighbor_port_num;

			neighbors[i].neighbor_fd = 0;
		}
		while(TRUE){
			printf("\n[PA1-Client@CSE489/589]$ ");
			fflush(stdout);

			char *msg = (char*) malloc(sizeof(char)*MSG_SIZE);
			memset(msg, '\0', MSG_SIZE);
			if(fgets(msg, MSG_SIZE-1, stdin) == NULL) //Mind the newline character that will be written to msg
				exit(-1);


			//printf("I got: %s(size:%d chars)", msg, strlen(msg));
			char author[] = "AUTHOR\n";
			int author_length = strlen(author);
			if (strcmp(msg, author) == 0) {
				cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
				cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", your_ubit_name);
				cse4589_print_and_log("[AUTHOR:END]\n");
			}
			char ip[] = "IP\n";
			if (strcmp(msg, ip) == 0) {
				cse4589_print_and_log("[IP:SUCCESS]\n");
				cse4589_print_and_log("IP:%s\n", host_ip);
				cse4589_print_and_log("[IP:END]\n");
			}


			char port[] = "PORT\n";
			if (strcmp(msg, port) == 0) {
				cse4589_print_and_log("[PORT:SUCCESS]\n");
				cse4589_print_and_log("PORT:%s\n", host_port);
				cse4589_print_and_log("[PORT:END]\n");
			}

			char login[] = "LOGIN\n";
			if(strstr(msg, login) == NULL) {
    				char* server_ip_addr;
    				char* temp = (char*) malloc(sizeof(char)*MSG_SIZE);
				char* server_port = (char*) malloc(sizeof(char)*MSG_SIZE);
				char delim[] = " ";
				temp = strtok(msg, delim);
				//printf(temp);
				//printf("\n");
				server_ip_addr = strtok(NULL, delim);
				//printf("server_ip_addr: %s\n", server_ip_addr);
				//printf("server_ip_addr len: %d\n", strlen(server_ip_addr));
				server_port = strtok(NULL, delim);
				//printf("host port: %s\n", host_port);
				//printf("host port len: %d\n", strlen(host_port));
				//printf("server_port: %s\n", server_port);
				//printf("server_port_len: %d\n", strlen(server_port));
				//printf(msg);
				//printf("strcmp host_port & server_port: %d\n", strcmp(host_port, server_port));
				char* trimmed_server_port = (char*) malloc(sizeof(char)*MSG_SIZE);
				strncpy(trimmed_server_port, server_port, strlen(server_port) - 1);
				server = connect_to_host(server_ip_addr, trimmed_server_port);
				
				char *ip_port = (char*) malloc(sizeof(char)*(2*MAXDATASIZE));
				memset(ip_port, '\0', 2*MAXDATASIZE);
				char space[] = " ";
				//printf("%s\n", host_port);
				strcat(space, host_port);
				//printf("%s\n", space);
				strcat(host_ip, space);
				//printf("%s\n", host_ip);
				//printf("\nSENDing it to the remote server ... ");
				send(server, host_ip, strlen(host_ip), 0);
				//for (int i = 0; i < BACKLOG - 2; i++) {
				char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
				memset(buffer, '\0', BUFFER_SIZE);
				printf("before recv\n");
				if(recv(server, buffer, BUFFER_SIZE, 0) >= 0){
						
				printf("Server responded: %s", buffer);
				/*	char curr_client_info_string[] = "client_info";
					if (strstr(buffer, curr_client_info_string) == NULL) {
						char* recv_curr_client_info = (char*) malloc(sizeof(char)*MSG_SIZE);
						char* recv_curr_client_ip_addr = (char*) malloc(sizeof(char)*MSG_SIZE);
						char* recv_curr_client_port_num = (char*) malloc(sizeof(char)*MSG_SIZE);
						char* recv_curr_client_fd = (char*) malloc(sizeof(char)*MSG_SIZE);
						char new_delim[] = " ";
						recv_curr_client_info = strtok(buffer, new_delim);
						recv_curr_client_ip_addr = strtok(NULL, new_delim);
						recv_curr_client_port_num = strtok(NULL, new_delim);
						recv_curr_client_fd = strtok(NULL, new_delim);
						printf("%s\n", recv_curr_client_info);
						printf("%s\n", recv_curr_client_ip_addr);
						printf("%s\n", recv_curr_client_port_num);
						printf("%s\n", recv_curr_client_fd);
					}*/
					
					free(buffer);
					//fflush(stdout);
				}
				//}
			}
			fflush(stdout);

			/* Initialize buffer to receieve response */
		}
	}
	return 0;
}

int connect_to_host(char *server_ip, char* server_port)
{
	int fdsocket;
	struct addrinfo hints, *res;

	/* Set up hints structure */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	/* Fill up address structures */
	if (getaddrinfo(server_ip, server_port, &hints, &res) != 0)
		perror("getaddrinfo failed");

	/* Socket */
	fdsocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(fdsocket < 0)
		perror("Failed to create socket");

	/* Connect */
	if(connect(fdsocket, res->ai_addr, res->ai_addrlen) < 0)
		perror("Connect failed");

	freeaddrinfo(res);

	return fdsocket;
}
