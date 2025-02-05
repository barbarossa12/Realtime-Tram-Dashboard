#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>

#define MAX_TRAMS 100

/* 
    The Tram data server (server.py) publishes messages over a custom protocol. 
    
    These messages are either:

    1. Tram Passenger Count updates (MSGTYPE=PASSENGER_COUNT)
    2. Tram Location updates (MSGTYPE=LOCATION)

    It publishes these messages over a continuous byte stream, over TCP.

    Each message begins with a 'MSGTYPE' content, and all messages are made up in the format of [CONTENT_LENGTH][CONTENT]:

    For example, a raw Location update message looks like this:

        7MSGTYPE8LOCATION7TRAM_ID7TRAMABC5VALUE4CITY

        The first byte, '7', is the length of the content 'MSGTYPE'. 
        After the last byte of 'MSGTYPE', you will find another byte, '8'.
        '8' is the length of the next content, 'LOCATION'. 
        After the last byte of 'LOCATION', you will find another byte, '7', the length of the next content 'TRAM_ID', and so on.

        Parsing the stream in this way will yield a message of:

        MSGTYPE => LOCATION
        TRAM_ID => TRAMABC
        VALUE => CITY

        Meaning, this is a location message that tells us TRAMABC is in the CITY.

        Once you encounter a content of 'MSGTYPE' again, this means we are in a new message, and finished parsing the current message

    The task is to read from the TCP socket, and display a realtime updating dashboard all trams (as you will get messages for multiple trams, indicated by TRAM_ID), their current location and passenger count.

    E.g:

        Tram 1:
            Location: Williams Street
            Passenger Count: 50

        Tram 2:
            Location: Flinders Street
            Passenger Count: 22

    To start the server to consume from, please install python, and run python3 server.py 8081

    Feel free to modify the code below, which already implements a TCP socket consumer and dumps the content to a string & byte array
*/

typedef struct {
    char tramID[128];
    char location[128];
    int passenger_count;

} TramInfo;

typedef struct {
    char messageType[128];
    char messageTypeValue[128];
    char tramID[128];
    char tramIDValue[128];
    char passengerCount[128];
    char locationString[128];
} tramDataPacket;

TramInfo trams[MAX_TRAMS];
int num_trams = 0;


int find_tram_index(char* tram_id);
void update_tram_info(char* tram_id, char* value, char* msgtype);
void print_dashboard();

void error(char* msg) {
    perror(msg);
    exit(1);
}

void dump_buffer(char* name) {
    int e;
    size_t len = strlen(name);
    for (size_t i = 0; i < len; i++) {
        e = name[i];
        printf("%-5d", e);
    }
    printf("\n\n");
    for (size_t i = 0; i < len; i++) {
        char c = name[i];
        if (!isalpha(name[i]) && !isdigit(name[i]) && (c != '_') && (c != ' '))
            c = '*';
        printf("%-5c", c);
    }
    printf("\n\n");
}

int main(int argc, char *argv[]){
	if(argc < 2){
        fprintf(stderr,"No port provided\n");
        exit(1);
	}	
	int sockfd, portno, n;
	char buffer[255];
	
	struct sockaddr_in serv_addr;
	struct hostent* server;
	
	portno = atoi(argv[1]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0){
		error("Socket failed \n");
	}
	
	server = gethostbyname("127.0.0.1");
	if(server == NULL){
		error("No such host\n");
	}
	
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	
	if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr))<0)
		error("Connection failed\n");
	
	while(1){
		bzero(buffer, 256);
		n = read(sockfd, buffer, 255);
		if(n<0)
			error("Error reading from Server");
		// dump_buffer(buffer);
        
        int i = 0;
        printf("*************************************\n");
        while (i < n) {

            // extract the message type information
            int content_length = buffer[i];
            i++;
            char msgtype[content_length + 1];
            strncpy(msgtype, buffer + i, content_length);
            msgtype[content_length] = '\0';
            i += content_length;
            
            // extract  the value based on the message type
            content_length = buffer[i];
            i++;
            char value[content_length + 1];
            strncpy(value, buffer + i, content_length);
            value[content_length] = '\0';
            i += content_length;

            // extract the tram ID field
            content_length = buffer[i];
            i++;
            char tram_id[content_length + 1];
            strncpy(tram_id, buffer + i, content_length);
            tram_id[content_length] = '\0';
            i += content_length;
            


            // tram ID value
            content_length = buffer[i];
            i++;
            char tram_id_value[content_length + 1];
            strncpy(tram_id_value, buffer + i, content_length);
            tram_id_value[content_length] = '\0';
            i += content_length;

            // value-padding
            content_length = buffer[i];
            i++;
            char value_padding[content_length + 1];
            strncpy(value_padding, buffer + i, content_length);
            value_padding[content_length] = '\0';
            i += content_length;

            // payload
            content_length = buffer[i];
            i++;
            char payload[content_length + 1];
            strncpy(payload, buffer + i, content_length);
            payload[content_length] = '\0';
            i += content_length;


            // printf("msgtype: %s\n", msgtype);
            // printf("msg_type_value: %s\n", value);
            // printf("tram_id: %s\n", tram_id);
            // printf("tram_id_value: %s\n", tram_id_value);
            // printf("value_padding: %s\n", value_padding);
            // printf("payload: %s\n", payload);
            
            if(strcmp(value, "LOCATION") == 0) {
                // location packet
                printf("location packet\n");
                update_tram_info(tram_id_value,payload,"LOCATION");
            }
            if(strcmp(value, "PASSENGER_COUNT") == 0) {
                // passenger count packet
                printf("passenger_count packet\n");
                update_tram_info(tram_id_value,payload,"PASSENGER_COUNT");
            }
            print_dashboard();
        }

        printf("*************************************\n");



	}
	
	return 0;
}

// Function to find tram by ID
int find_tram_index(char* tram_id) {
    for (int i = 0; i < num_trams; i++) {
        if (strcmp(trams[i].tramID, tram_id) == 0) {
            return i;
        }
    }
    return -1;
}

// Function to update tram info
void update_tram_info(char* tram_id, char* value, char* msgtype) {
    int tram_index = find_tram_index(tram_id);
    
    if (tram_index == -1) {
        // If tram not found, add it to the list
        strcpy(trams[num_trams].tramID, tram_id);
        tram_index = num_trams;
        num_trams++;
    }

    if (strcmp(msgtype, "LOCATION") == 0) {
        // Update location for the tram
        strcpy(trams[tram_index].location, value);
    } else if (strcmp(msgtype, "PASSENGER_COUNT") == 0) {
        // Update passenger count for the tram
        trams[tram_index].passenger_count = atoi(value);
    }
}

// Function to print the dashboard
void print_dashboard() {
    printf("\n\nRealtime Tram Dashboard\n");
    for (int i = 0; i < num_trams; i++) {
        printf("Tram %d:\n", i+1);
        printf("    Location: %s\n", trams[i].location);
        printf("    Passenger Count: %d\n\n", trams[i].passenger_count);
    }
}


