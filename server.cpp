#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <bits/stdc++.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>

#include <arpa/inet.h>

#include <string.h>
#include <unistd.h>
#include "utils.h"

using namespace std;

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int sockfd_tcp, sockfd_udp;
    char buffer[NMAX];
    fd_set active_sockfds;
    fd_set tmp_sockfds;

    struct sockaddr_in udp_addr;
    struct sockaddr_in tcp_addr;

    socklen_t socklen_tcp = sizeof(struct sockaddr_in);
    socklen_t socklen_udp = sizeof(struct sockaddr_in);
    
    // set with ids of active clients  
    unordered_set<string> active_clients_by_id;

    // active clients map where key = socketfd, value = id.
    unordered_map<int, string> active_clients_by_socketfd;

    // active clients map were key = id, value = socketfd.
    unordered_map<string, int> id_socket;

    // map that stores the subscribed ids (with their sf) to a specific topic
    // key = topic, value = map (with key = id client, value = sf)   
    unordered_map<string, unordered_map<string, int>> topic_client_sf;

    // the messages of topics with sf = 1 since a client became offline.
    // key = id_client, value = vector of messages.  
    unordered_map<string, vector<struct tcp_message>> buffers;

    
    DIE(argc != 2, "Wrong number of arguments!\n");  

    // Create the TCP socket 
    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_tcp < 0, "socket_tcp");

    // Create the UDP socket
    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_udp < 0, "socket_udp");

    // Server port number 
    int port_number = atoi(argv[1]);
    DIE(port_number == 0, "atoi");

    // Complete the fields of udp_addr 
    memset((char *) &udp_addr, 0, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(port_number);
	udp_addr.sin_addr.s_addr = INADDR_ANY;

    // Complete the fields of tcp_addr
    memset((char *) &tcp_addr, 0, sizeof(tcp_addr));
	tcp_addr.sin_family = AF_INET;
	tcp_addr.sin_port = htons(port_number);
	tcp_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind TCP
    int ret = bind(sockfd_tcp, (struct sockaddr *) &tcp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind -- tcp");

    // Bind UDP
    ret = bind(sockfd_udp, (struct sockaddr *) &udp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind -- udp");

    // Listen TCP
    ret = listen(sockfd_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen");

    // Set the fds of udp and tcp sockets.
    FD_ZERO(&active_sockfds);
    FD_ZERO(&tmp_sockfds);

    FD_SET(sockfd_udp, &active_sockfds);
    FD_SET(sockfd_tcp, &active_sockfds);
    FD_SET(STDIN_FILENO, &active_sockfds);

    int fdmax = sockfd_tcp;
    bool is_exit = false;
    
    while (!is_exit) {
        memset(buffer, 0, sizeof(buffer));
        tmp_sockfds = active_sockfds;
        		
		ret = select(fdmax + 1, &tmp_sockfds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_sockfds)) {
				if (i == STDIN_FILENO) {
                    // receives command from stdin
                    fgets(buffer, NMAX, stdin);

                   
                    if (!strcmp(buffer, "exit\n")) {
                        is_exit = true;
                        break;
                    } 

                    
                } else if (i == sockfd_tcp) {
                    struct sockaddr_in new_tcp_addr;
                    int new_client_socketfd = accept(i, (sockaddr *) &new_tcp_addr, &socklen_tcp);
                    DIE(new_client_socketfd < 0, "accept()");

                    // disable the Neagle for accepted socket fd  
                    int flag = 1;
                    setsockopt(new_client_socketfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

                    FD_SET(new_client_socketfd, &active_sockfds);
                    fdmax = max(fdmax, new_client_socketfd);
                   
                    
                    ret = recv(new_client_socketfd, buffer, 11, 0);
                    DIE(ret < 0, "recv()");
                    
                    // if client isn't now connected. 
                    if (active_clients_by_id.find(buffer) == active_clients_by_id.end()) {
                        // set client as online     
                        active_clients_by_socketfd[new_client_socketfd] = buffer;
                        active_clients_by_id.insert(buffer);
                        id_socket[buffer] = new_client_socketfd;

                        send_saved_messages(new_client_socketfd, buffer, buffers);
                                        
                        printf("New client %s connected from %s:%d.\n", buffer,
                           inet_ntoa(new_tcp_addr.sin_addr), ntohs(new_tcp_addr.sin_port));

                    // if client is already connected.
                    } else {
                        printf("Client %s already connected.\n", buffer);
                        FD_CLR(new_client_socketfd, &active_sockfds);
                        close(new_client_socketfd);
                    }

                    

				} else if (i == sockfd_udp) {
                    recvfrom(sockfd_udp, buffer, sizeof(struct udp_message), 0, (sockaddr *) &udp_addr, &socklen_udp);

                    // message to be sent to client
                    struct tcp_message sent_message;

                    strcpy(sent_message.ip, inet_ntoa(udp_addr.sin_addr));
                    sent_message.port_client_udp = ntohs(udp_addr.sin_port);

                    // message received from udp client.
                    struct udp_message *received_message = (struct udp_message*) buffer;                    

                    convert_message(received_message, &sent_message);
                    send_or_store_udp_messages(sent_message, topic_client_sf, buffers, active_clients_by_id, id_socket);

				} else {
                 	
					int n = recv(i, buffer, sizeof(struct server_message), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// conexiunea s-a inchis
						cout << "Client " << active_clients_by_socketfd[i] << " disconnected.\n";

                        // Set client as offline (remove it from maps/sets regardingg online clients).  
                        active_clients_by_id.erase(active_clients_by_socketfd[i]);
                        id_socket.erase(active_clients_by_socketfd[i]);
                        active_clients_by_socketfd.erase(i);
                        
                        FD_CLR(i, &active_sockfds);
                        close(i);

                        update_fdmax(fdmax, &active_sockfds);
						
					} else {
                        struct server_message *received_message = (struct server_message *) buffer;

                        if (received_message->type) {
                            // Subscribe
                            topic_client_sf[received_message->topic][active_clients_by_socketfd[i]] = received_message->sf;
                        } else {
                            // Unsubscribe
                            topic_client_sf[received_message->topic].erase(active_clients_by_socketfd[i]);
                        }

                    }  
				}
                
            }       
  
        }
    }

    close_all_sockets(fdmax, &active_sockfds);

    return 0;
}