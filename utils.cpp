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

void preprocess_command(int server_socket, char *command) {
    command[strlen(command) - 1] = '\0';
    struct server_message sent_message;
    memset(sent_message.topic, 0, sizeof(sent_message.topic));  
    int ret;


    char delim[] = " \n";
    char *word = strtok(command, delim);
    
    if (!strcmp(word, "subscribe")) {
        sent_message.type = 1; // subscribe
        word = strtok(NULL, delim);
        
        if (word) {
            strcpy(sent_message.topic, word);

            word = strtok(NULL, delim);

            if (word) {
                if (word[0] != '0' && word[0] != '1') {
                    fprintf(stderr, "SF must be either 0 or 1\n");
                    return;
                }

                sent_message.sf = atoi(word);
                word = strtok(NULL, delim);

                if (!word) {
                    ret = send(server_socket, (char *) &sent_message, sizeof (sent_message), 0);
                    DIE(ret < 0, "send()");

                    printf("Subscribed to topic.\n");
                } else {
                    fprintf(stderr, "Too many arguments for subscribe command.\n");                    
                }
            } else {
                fprintf(stderr, "Too few arguments for subscribe command. Try one more!\n");
            }
        } else {
             fprintf(stderr, "Too few arguments for subscribe command. Try two more!\n");
        }

    } else if (!strcmp(word, "unsubscribe")) {
        sent_message.type = 0; // subscribe
        word = strtok(NULL, delim);
        
        if (word) {
            strcpy(sent_message.topic, word);
            word = strtok(NULL, delim);

            if (!word) {
                sent_message.sf = 0;
                ret = send(server_socket, (char *) &sent_message, sizeof (sent_message), 0);
                DIE(ret < 0, "send()");

                printf("Unsubscribed from topic.\n");
            } else {
                fprintf(stderr, "Too many arguments for subscribe command.\n"); 
            }
           
        } else {
            fprintf(stderr, "Too few arguments for unsubscribe command. Try one more!\n");
        }
        
    } else {
        fprintf(stderr, "First word of command is not correct!\n");
    }   

}

void update_fdmax(int fdmax, fd_set *active_sockfds) {
    int copy_fdmax = fdmax;

    for (int j = 3; j < copy_fdmax; j++) {
        if (FD_ISSET(j, active_sockfds)) {
            fdmax = max(j, fdmax);
        }    
    }
}

void close_all_sockets(int fdmax, fd_set *active_sockfds) {
    for (int i = 3; i <= fdmax; i++) {
        if (FD_ISSET(i, active_sockfds)) {
            close(i);
        } 
    }
}

void send_saved_messages(int new_client_socketfd, char buffer[NMAX], unordered_map<string, vector<struct tcp_message>> &buffers) {
    vector<tcp_message> saved_messages = buffers[buffer];

    for (auto message : saved_messages) {
        send(new_client_socketfd, (char*) &message, sizeof(struct tcp_message), 0);
    }

    buffers.erase(buffer);
} 

void send_or_store_udp_messages(struct tcp_message &sent_message, unordered_map<string, unordered_map<string, int>> &topic_client_sf,
                                    unordered_map<string, vector<struct tcp_message>> &buffers, unordered_set<string> &active_clients_by_id,
                                    unordered_map<string, int> &id_socket) {
    for (auto id_sf : topic_client_sf[sent_message.topic]) {
        // Send message to all online clients   
        if (active_clients_by_id.find(id_sf.first) != active_clients_by_id.end()) {
            send(id_socket[id_sf.first], (char*) &sent_message, sizeof(sent_message), 0);

            // Save messages for each offline user with that specific topic with sf = 1
        } else if (id_sf.second == 1) {
            if (buffers.find(id_sf.first) == buffers.end()) {
                vector<struct tcp_message> messages;
                messages.push_back(sent_message);

                buffers[id_sf.first] = messages; 
            } else {
                buffers[id_sf.first].push_back(sent_message);
            }
        }
    }
}

void convert_message(struct udp_message *received_message, struct tcp_message *sent_message) {
    DIE(received_message->type_name > 3 || received_message->type_name < 0, "Invalid conversion type");

    
    uint8_t type = received_message->type_name;

    strncpy(sent_message->topic, received_message->topic_name, 50);
    sent_message->topic[50] = '\0';


    // INT
    if (type == 0) {
        DIE(received_message->payload[0] > 1, "First byte is not a sign octet!\n");
        long long int_number = ntohl(*(uint32_t *) (received_message->payload + 1));

        if (received_message->payload[0] != 0) {
            int_number = -int_number;
        }

        sprintf(sent_message->payload, "%lld", int_number);
        strcpy(sent_message->data_type, "INT");

        // SHORT_REAL
    } else if (type == 1) {
        double real_number = (double) ntohs(*(uint16_t *) (received_message->payload)) / 100;
        sprintf(sent_message->payload, "%.2f", real_number);
        strcpy(sent_message->data_type, "SHORT_REAL");
        
        // FLOAT
    } else if (type == 2) {
        DIE(received_message->payload[0] > 1, "First byte is not a sign octet!\n");

        double real_number = ntohl(*(uint32_t *) (received_message->payload + 1)) / pow(10, received_message->payload[5]);

        if (received_message->payload[0] != 0) {
            real_number = -real_number;
        }

        sprintf(sent_message->payload, "%lf", real_number);
        strcpy(sent_message->data_type, "FLOAT");

        // STRING
    } else {
        strcpy(sent_message->payload, received_message->payload);    
        strcpy(sent_message->data_type, "STRING");  

    }
}