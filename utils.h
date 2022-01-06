#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unordered_set>
#include <string>
#include <unordered_map>

using namespace std;

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define NMAX 1600
#define MAX_CLIENTS INT_MAX

struct __attribute__((packed)) udp_message {
    char topic_name[50];
    uint8_t type_name;
    char payload[1501];
}; 

struct __attribute__((packed)) tcp_message {
    char ip[17];
    uint16_t port_client_udp;
    char topic[51];
    char data_type[11];
    char payload[1501];
};

struct __attribute__((packed)) server_message {
    char topic[51];
    int sf;   // 0 or 1 
    int type; // 1 -> subscribe, 0 -> unsubscribe
};

// Preprocess command of subscribe/unsubscribe and send it to server 
void preprocess_command(int server_socket, char *command);

// update the max fd
void update_fdmax(int fdmax, fd_set *active_sockfds);

// close all the fds, after the exit command is executed in server.
void close_all_sockets(int fdmax, fd_set *active_sockfds);

// send the saved messages from topics with sf = 1 for a reconnected client.
void send_saved_messages(int new_client_socketfd, char buffer[NMAX], unordered_map<string, vector<struct tcp_message>> &buffers);

// convert the message from udp format to tcp one.
void convert_message(struct udp_message *received_message, struct tcp_message *sent_message);

// send the udp messages of specific topic to online clients and store the ones of offline clients with sf = 1.
void send_or_store_udp_messages(struct tcp_message &sent_message, unordered_map<string, unordered_map<string, int>> &topic_client_sf,
                                    unordered_map<string, vector<struct tcp_message>> &buffers, unordered_set<string> &active_clients_by_id,
                                    unordered_map<string, int> &id_socket);

#endif  // UTILS_H