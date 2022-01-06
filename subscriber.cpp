#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"

using namespace std;


int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    
    struct sockaddr_in serv_addr;
    int port_number_server = atoi(argv[3]);
    char buffer[NMAX];  
    
    fd_set read_fds, tmp_fds;
    
    DIE(argc != 4, "Incorrect number of arguments");
    DIE(strlen(argv[1]) > 10, "strlen of id_client"); 
    
    // Create socket for server.
    int sockfd_serv = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd_serv < 0, "socket()");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_number_server);
    int ret = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(ret == 0, "inet_aton()");

    // Connect to server 
    ret = connect(sockfd_serv, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect()");

    // Disable the Neagle algorithm.
    int flag = 1;
    setsockopt(sockfd_serv, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    // Send the id of client.
    ret = send(sockfd_serv, argv[1], strlen(argv[1]) + 1, 0);
    DIE(ret < 0, "send id");
    
    // Set fds for stdin and server socket 
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(sockfd_serv, &read_fds);
	
    int fdmax = sockfd_serv;
           
    while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select()");

        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			memset(buffer, 0, NMAX);
            // read command from stdin
			fgets(buffer, NMAX, stdin);

			if (!strcmp(buffer, "exit\n")) {
				break;
			}

            preprocess_command(sockfd_serv, buffer);

		} else if (FD_ISSET(sockfd_serv, &tmp_fds)) {
            memset(buffer, 0, NMAX);
            // receive messae from server
			int bytes = recv(sockfd_serv, buffer, sizeof(struct tcp_message), 0);
            DIE(bytes < 0, "recv()");

            // check if server closes the connection.
            if (!bytes) {
				break;
			}

            struct tcp_message *receive_message = (struct tcp_message *) buffer;
            printf("%s:%d - %s - %s - %s\n", receive_message->ip, receive_message->port_client_udp,
                   receive_message->topic, receive_message->data_type, receive_message->payload);
          
        }
    }    

    close(sockfd_serv); 

    return 0;
}    
