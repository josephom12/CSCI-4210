/*
	Joseph Om, Spring 2020
	Homework 4 - Network Programming and Multi-threaded Programming using C
	Score: 100/100
*/

#define MAXBUFFER 1100

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/select.h>   
#include <ctype.h> 

// Global variables 
int* client_sockets;
char** active_users;
int active_user_count;	

// Mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to find the next avaliable client_socket/active_user index
int findNextSocketIndex(){
	int i;
	for (i = 0; i < 32; i++){
		if (client_sockets[i] == -1){
			return i;
		}
	}
	return i;
}

// Function to check if a userid is already connected
int isUserConnected(char* userid){
	pthread_mutex_lock(&mutex);
	int i;
	for (i = 0; i < 32; i++){
		if (strcmp(userid, active_users[i]) == 0){
			pthread_mutex_unlock(&mutex);
			return 1;
		}
	}
	pthread_mutex_unlock(&mutex);
	return 0;
}

// Function to check if a character array is alphanumeric 
int isAlphanumeric(char* message){
	int i;
	for (i = 0; i < strlen(message); i++){
		if (!isalnum(message[i]) && message[i] != '\n'){
			return 0;
		}
	}
	return 1;
}

// Function to handle the LOGIN command
void handleLogin(int fd, char* recievedMessage, int curr_socket_index) {
	char* res = calloc(MAXBUFFER, sizeof(char));

	char* userid = recievedMessage + 6;
	int length = strlen(userid) - 1;	
	userid[length] = '\0';

	printf("CHILD %ld: Rcvd LOGIN request for userid %s\n", pthread_self(), userid);

	// In the case of an invalid userid
	if (length < 4 || length > 16 || !isAlphanumeric(userid)){
		strcpy(res, "ERROR Invalid userid\n");
		printf("CHILD %ld: Sent ERROR (Invalid userid)\n", pthread_self());
	}

	// In the case the user is already connected 
	else if (isUserConnected(userid)){
		strcpy(res, "ERROR Already connected\n");
		printf("CHILD %ld: Sent ERROR (Already connected)\n", pthread_self());
	}

	// In the case of a valid USERID 
	else {

		pthread_mutex_lock(&mutex);

		active_users[curr_socket_index] = realloc(active_users[curr_socket_index], sizeof(char) * (strlen(userid) + 1));
		strcpy(active_users[curr_socket_index], userid); 
		active_users[curr_socket_index][strlen(userid)] = '\0';
		active_user_count += 1;
		
		pthread_mutex_unlock(&mutex);

		strcpy(res, "OK!\n");
	}

	send(fd, res, strlen(res), 0);
	return;
}

// Function to handle the LOGOUT command (TCP)
void handleLogout(int fd, char* recievedMessage, int curr_socket_index) {

	printf("CHILD %ld: Rcvd LOGOUT request\n", pthread_self());

	char* res = calloc(MAXBUFFER, sizeof(char));
	strcpy(res, "OK!\n");

	pthread_mutex_lock(&mutex);

	// In the case a user is currently logged in this TCP connection
	if (strcmp(active_users[curr_socket_index], ".") != 0){
		active_user_count -= 1;
		free(active_users[curr_socket_index]);
		active_users[curr_socket_index] = calloc(2, sizeof(char));
		strcpy(active_users[curr_socket_index], ".");
	}

	pthread_mutex_unlock(&mutex);

	send(fd, res, strlen(res), 0);
}


static int compare(const void* a, const void* b){
	return strcmp(*(const char**)a, *(const char**)b);
}

void sort( char* arr[], int n){
	qsort(arr, n, sizeof(const char*), compare);
}

// Function to handle the WHO command
void handleWho(int fd, int TCP_UDP, int sd_udp, struct sockaddr_in client, int len) {

	// TCP 
	if (TCP_UDP == 1){
		printf("CHILD %ld: Rcvd WHO request\n", pthread_self());
	}
	// UDP
	else if (TCP_UDP == 2){
		printf("MAIN: Rcvd WHO request\n");
	}

	char* res = calloc(MAXBUFFER, sizeof(char));
	res[0] = 'O';
	res[1] = 'K';
	res[2] = '!';
	res[3] = '\n';

	// In the case that there are no users currently logged in
	if (active_user_count == 0){

		if (TCP_UDP == 1){
			send(fd, res, strlen(res), 0);
		}
		else if (TCP_UDP == 2){
			sendto(sd_udp, res, strlen(res), 0, (struct sockaddr* ) &client, len);
		}
		return;
	}

	pthread_mutex_lock(&mutex);

	// Create a sorted version of the array 
	char** sorted_active_users = calloc(32, sizeof(char*));
	int i, j;
	for (i = 0; i < 32; i++){
		sorted_active_users[i] = calloc(strlen(active_users[i]) + 1, sizeof(char));
		strcpy(sorted_active_users[i], active_users[i]);
	}

	sort(sorted_active_users, active_user_count);


	// Add sorted userids to res
	int offset = 4;
	for (i = 0; i < 32; i++){
		if (*sorted_active_users[i] == '.'){
			continue;
		}
		for (j = 0; j < strlen(sorted_active_users[i]); j++){
			res[offset] = sorted_active_users[i][j];
			offset += 1;
		}
		res[offset] = '\n';
		offset += 1;
	}

	res[offset] = '\0';

	pthread_mutex_unlock(&mutex);
	if (TCP_UDP == 1){
		send(fd, res, strlen(res), 0);
	}
	else if (TCP_UDP == 2){
		sendto(sd_udp, res, strlen(res), 0, (struct sockaddr* ) &client, len);
	}
	return;
}

// Function to handle the SEND command (TCP)
void handleSend(int fd, char* recievedMessage, int curr_socket_index, int TCP_UDP) {

	// In the case this command was sent from a UDP packet
	if (TCP_UDP == 2){
		printf(" Sent ERROR (SEND not supported over UDP)\n");
		send(fd, "ERROR SEND not supported over UDP\n", 34, 0);
		return;
	}

	char* afterSend; 

	// Handle multiple packets
	if (strlen(recievedMessage) <= 5){
		char buffer[MAXBUFFER] = {'\0'};
		recv(client_sockets[curr_socket_index], buffer, MAXBUFFER - 1, 0);			
		afterSend = buffer;
	}
	else {
		afterSend = recievedMessage + 5;
	}

	int msgLen;
	char* messageToSend; 
	int i; 

	// Parse recipient userid 
	for (i = 0; i < strlen(afterSend); i++){
		if (afterSend[i] == ' ' || afterSend[i] == '\n'){
			break;
		}
	}
	char recipientUserId[MAXBUFFER] = {'\0'}; 
	memcpy(recipientUserId, afterSend, i);
	recipientUserId[i] = '\0';	

	printf("CHILD %ld: Rcvd SEND request to userid %s\n", pthread_self(), recipientUserId);

	// Parse message length 
	int length_count = 0;
	for (i = i + 1; i < strlen(afterSend); i++){
		if (afterSend[i] == '\n' || afterSend[i] == ' '){
			break;
		}
		length_count += 1;
	}

	char msgLenChar[4] = {'\0'};

	memcpy(msgLenChar, afterSend + strlen(recipientUserId) + 1, length_count);
	msgLenChar[length_count] = '\0'; 

	msgLen = atoi(msgLenChar);

	// Handle multiple packets
	if (strlen(recievedMessage) <= (strlen(msgLenChar) + strlen(recipientUserId) + 4 + 2 + 1)){
		char buffer[MAXBUFFER] = {'\0'};
		recv(client_sockets[curr_socket_index], buffer, MAXBUFFER - 1, 0);			
		messageToSend = buffer;
	}
	else {
		messageToSend = afterSend + strlen(msgLenChar) + strlen(recipientUserId) + 2;
	}

	// Validate that the user is currently logged in
	if (!isUserConnected(recipientUserId)){
		printf("CHILD %ld: Sent ERROR (Unknown userid)\n", pthread_self());
		send(fd, "ERROR Unknown userid\n", 21, 0);
		return;
	}

	// Validate the SEND format

	// Validate that the message length is appropriate 
	else if (msgLen < 1 || msgLen > 990){
		printf("CHILD %ld: Sent ERROR (Invalid msglen)\n", pthread_self());
		send(fd, "ERROR Invalid msglen\n", 21, 0);
		return;
	}

	// Send the message
	else {

		pthread_mutex_lock(&mutex);
		char senderUserId[MAXBUFFER] = {'\0'};	
		memcpy(senderUserId, active_users[curr_socket_index], strlen(active_users[curr_socket_index]));
		pthread_mutex_unlock(&mutex);

		char* resultMessage = calloc( 5 + strlen(senderUserId) + strlen(msgLenChar) + strlen(messageToSend) + 1, sizeof(char));
		int resIndex = 5; 

		strcpy(resultMessage, "FROM ");

		int i;
		for (i = 0; i < strlen(senderUserId); i++){
			resultMessage[resIndex] = senderUserId[i];
			resIndex += 1;
		}

		resultMessage[resIndex] = ' ';
		resIndex += 1;

		for (i = 0; i < strlen(msgLenChar); i++){
			resultMessage[resIndex] = msgLenChar[i];
			resIndex += 1;
		}
		resultMessage[resIndex] = ' ';
		resIndex += 1;

		for (i = 0; i < strlen(messageToSend); i++){
			resultMessage[resIndex] = messageToSend[i];
			resIndex += 1;
		}

		resultMessage[resIndex] = '\0';

		// Find the userid's FD to send the message to 
		int sendIndex;
		pthread_mutex_lock(&mutex);
		for (i = 0; i < 32; i++){
			if (strcmp(active_users[i], recipientUserId) == 0){
				sendIndex = i;
				break;
			}
		}
		pthread_mutex_unlock(&mutex);

		char resultMessageChar[MAXBUFFER] = {'\0'};
		memcpy(resultMessageChar, resultMessage, strlen(resultMessage));
		free(resultMessage);

		send(fd, "OK!\n", 4, 0);
		send(client_sockets[sendIndex], resultMessageChar, strlen(resultMessageChar), 0);
		return;
	}
}

// Function to handle the BROADCAST command (TCP/UDP)
void handleBroadcast(int fd, char* recievedMessage, int curr_socket_index, int TCP_UDP, int sd_udp, struct sockaddr_in client, int len) {
	
	// TCP 
	if (TCP_UDP == 1){
		printf("CHILD %ld: Rcvd BROADCAST request\n", pthread_self());
	}
	// UDP
	else if (TCP_UDP == 2){
		printf("MAIN: Rcvd BROADCAST request\n");
	}

	// Parse BROADCAST command 
	char* afterBroadcast = recievedMessage + 10;

	char* messageToSend;
	int msgLen;

	int i;
	int length_count = 0;
	for (i = 0; i < strlen(afterBroadcast); i++){
		if (afterBroadcast[i] == '\n' || afterBroadcast[i] == ' '){
			break;
		}
		length_count += 1;
	}	

	char msgLenChar[4] = { '\0' };
	memcpy(msgLenChar, afterBroadcast, length_count);
	msgLenChar[length_count] = '\0';
	msgLen = atoi(msgLenChar);

	// Validate that the message length is appropriate 
	if (msgLen < 1 || msgLen > 990){
		printf("CHILD %ld: Sent ERROR (Invalid msglen)\n", pthread_self());
		send(fd, "ERROR Invalid msglen\n", 21 , 0);
		return;
	}

	// Handle multiple packets
	if (strlen(recievedMessage) <= (strlen(msgLenChar) + 11)){
		char buffer[MAXBUFFER] = {'\0'};
		recv(client_sockets[curr_socket_index], buffer, MAXBUFFER - 1, 0);	
		messageToSend = buffer;
	}
	else {
		messageToSend = afterBroadcast + strlen(msgLenChar) + 1;
	}

	// Parse message to send 
	char* resultMessage = calloc(MAXBUFFER, sizeof(char));
	int resIndex = 5;
	strcpy(resultMessage, "FROM ");

	char senderUserId1[MAXBUFFER] = {'\0'};

	// TCP
	if (TCP_UDP == 1){
		pthread_mutex_lock(&mutex);

		memcpy(senderUserId1, active_users[curr_socket_index], strlen(active_users[curr_socket_index]));
	
		pthread_mutex_unlock(&mutex);

		int i;
		for (i = 0; i < strlen(senderUserId1); i++){
			resultMessage[resIndex] = senderUserId1[i];
			resIndex += 1;
		}
		resultMessage[resIndex] = ' ';
		resIndex += 1;
	}

	// UDP
	else if (TCP_UDP == 2) {
		char* temp = "UDP-client ";
		for (i = 0; i < strlen(temp); i++){
			resultMessage[resIndex] = temp[i];
			resIndex += 1;
		}
	}

	for (i = 0; i < strlen(msgLenChar); i++){
		resultMessage[resIndex] = msgLenChar[i];
		resIndex += 1;
	}

	resultMessage[resIndex] = ' ';
	resIndex += 1;

	for (i = 0; i < strlen(messageToSend); i++){
		resultMessage[resIndex] = messageToSend[i];
		resIndex += 1;
	}

	if (resultMessage[resIndex - 1] != '\n'){
		resultMessage[resIndex] = '\n';
		resIndex += 1;
	}
	resultMessage[resIndex] = '\0';

	if (TCP_UDP == 1){
		send(fd, "OK!\n", 4, 0);
	}
	else if (TCP_UDP == 2){
		sendto(sd_udp, "OK!\n", 4, 0, (struct sockaddr* ) &client, len);
	}

	char resultMessageChar[MAXBUFFER] = {'\0'};
	memcpy(resultMessageChar, resultMessage, strlen(resultMessage));
	free(resultMessage);

	// Send message to all active users
	pthread_mutex_lock(&mutex);
	for (i = 0; i < 32; i++){
		if (strcmp(active_users[i], ".") != 0){
			send(client_sockets[i], resultMessageChar, strlen(resultMessageChar), 0);
		}
	}
	pthread_mutex_unlock(&mutex);

	return;
}

// Function to handle all messages recieved in both TCP and UDP servers; 1 -> TCP, 2 -> UDP
void handleMessage(int fd, char* recievedMessage, int curr_socket_index, int TCP_UDP, int sd_udp, struct sockaddr_in client, int len) {

	if (strncmp(recievedMessage, "LOGIN", 5) == 0){
		handleLogin(fd, recievedMessage, curr_socket_index);
	}

	else if (strncmp(recievedMessage, "WHO", 3) == 0){
		handleWho(fd, TCP_UDP, sd_udp, client, len);
	}

	else if (strncmp(recievedMessage, "LOGOUT", 6) == 0){
		handleLogout(fd, recievedMessage, curr_socket_index);
	}

	else if (strncmp(recievedMessage, "SEND", 4) == 0){
		handleSend(fd, recievedMessage, curr_socket_index, TCP_UDP);
	}

	else if (strncmp(recievedMessage, "BROADCAST", 9) == 0){
		handleBroadcast(fd, recievedMessage, curr_socket_index, TCP_UDP, sd_udp, client, len);
	}

	return;
}

// Arguments for processNewConnection function 
struct processNewConnectionArgs{
	int fd; 
	fd_set* rset;
	struct sockaddr_in* client; 
	int curr_socket_index;
	pthread_t curr_thread;
};

// Function to handle a new connection in TCP
void* processNewConnection(void* args){

	struct processNewConnectionArgs* args1 = args;

	int fd = args1->fd;
	struct sockaddr_in client = *args1->client;
	int curr_socket_index = args1->curr_socket_index;

	while (1){

		char buffer[MAXBUFFER];
		int n = recv(fd, buffer, MAXBUFFER - 1, 0);	

		if (n < 0){
			perror("ERRROR");
		}

		// In the case the connection was closed 
		else if (n == 0) {
			printf(" Client disconnected\n");
			
			close(fd);

			pthread_mutex_lock(&mutex);

			// Remove this FD
			client_sockets[curr_socket_index] = -1; 

			// Remove this user
			if (strcmp(active_users[curr_socket_index], ".") != 0){
				active_user_count -= 1;
				free(active_users[curr_socket_index]);
				active_users[curr_socket_index] = calloc(2, sizeof(char));
				strcpy(active_users[curr_socket_index], ".");
			}

			pthread_mutex_unlock(&mutex);
			free(args);
			return NULL;
		}

		// In the case that the TCP connection has recived a message
		else{
			buffer[n] = '\0';
			handleMessage(fd, buffer, curr_socket_index, 1, -1, client, -1);
		}
	}
	return NULL;
}

int main(int argc, char** argv) {

	setvbuf( stdout, NULL, _IONBF, 0 );

 	if (argc < 2){
 		fprintf(stderr, "MAIN: ERROR Invalid number of arguments");
 		return EXIT_FAILURE; 
 	}

	int port_number = atoi(argv[1]);

	int sd_tcp; 
	sd_tcp = socket( PF_INET, SOCK_STREAM, 0);
	if (sd_tcp == -1){
		perror("MAIN: ERROR TCP Socket failed");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_tcp; 
	server_tcp.sin_family = PF_INET; 
	server_tcp.sin_addr.s_addr = htonl(INADDR_ANY);
	server_tcp.sin_port = htons(port_number);
	
	int len = sizeof(server_tcp);
	
	if (bind(sd_tcp, (struct sockaddr* )&server_tcp, len) == -1){
		perror("MAIN: ERROR bind() failed");
		return EXIT_FAILURE;
	}

	if (listen(sd_tcp, 32) == -1){
		perror("MAIN: ERROR listen() failed");
		return EXIT_FAILURE;
	}

	// Create the socket (endpoint) on the server side for UDP 
	int sd_udp; 
	sd_udp = socket( AF_INET, SOCK_DGRAM, 0);
	if (sd_udp == -1){
		perror("MAIN: ERROR UDP Socket failed");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_udp;
	server_udp.sin_family = AF_INET;
	server_udp.sin_addr.s_addr = htonl(INADDR_ANY);
	server_udp.sin_port = htons(port_number);

	if (bind(sd_udp, (struct sockaddr* ) &server_udp, sizeof(server_udp)) < 0){
		perror("MAIN: ERROR bind() failed");
		return EXIT_FAILURE;
	}

	// Output start 
	printf("MAIN: Started server\n");
	printf("MAIN: Listening for TCP connections on port: %d\n", port_number);
	printf("MAIN: Listening for UDP datagrams on port: %d\n", port_number);

	fd_set rset; 

	char buffer[MAXBUFFER];
	pthread_t tid[32];

	int n;
	struct sockaddr_in client; 
	len = sizeof(client);

	pthread_mutex_lock(&mutex);

	// Initialize client_sockets: Container to hold all FDs for TCP connections
	int k;
	client_sockets = calloc(32, sizeof(int));
	for (k = 0; k < 32; k++){
		client_sockets[k] = -1;
	}

	// Initialize active_users: Container to hold all active userids 
	active_user_count = 0;
	active_users = calloc(32, sizeof(char*));
	int l; 
	for (l = 0; l < 32; l++){
		active_users[l] = calloc(2, sizeof(char));
		strcpy(active_users[l], ".");
	}

	pthread_mutex_unlock(&mutex);

	// Listen for TCP/UDP connections/datagrams
	while (1){

		FD_ZERO(&rset);
		FD_SET(sd_udp, &rset);
		FD_SET(sd_tcp, &rset);

		int ready = select(FD_SETSIZE, &rset, NULL, NULL, NULL);
		if (ready == -1){
			perror("MAIN: ERROR select() failed");
			return EXIT_FAILURE;
		}

		// In the case the TCP socket detected a new connection
		if (FD_ISSET(sd_tcp, &rset)){

			int client_socket_index = findNextSocketIndex();

			int new_socket = accept(sd_tcp, (struct sockaddr*) &client, (socklen_t*) &len);
			client_sockets[client_socket_index] = new_socket;

			struct processNewConnectionArgs* args = malloc(sizeof(struct processNewConnectionArgs)); 
			args->fd = client_sockets[client_socket_index];
			args->rset = &rset;
			args->client = &client;
			args->curr_socket_index = client_socket_index;
			args->curr_thread = tid[client_socket_index];

			printf("MAIN: Rcvd incoming TCP connection from: %s\n", inet_ntoa((struct in_addr) client.sin_addr));

			pthread_create(&tid[client_socket_index], NULL, &processNewConnection, args);
		}

		// In the case the UDP socket has recieved a message
		if (FD_ISSET(sd_udp, &rset)){
			n = recvfrom(sd_udp, buffer, sizeof(buffer), 0, (struct sockaddr *) &client, (socklen_t* ) &len );

			if (n == -1){
				perror("MAIN: ERROR recvfrom() failed");
			}
			else{
				buffer[n] ='\0';
				printf("MAIN: Rcvd incoming UDP datagram from: %s\n", inet_ntoa((struct in_addr) client.sin_addr));
				handleMessage(-1, buffer, -1, 2, sd_udp, client, len);
			}
		}
	}

	pthread_mutex_destroy(&mutex);
	return EXIT_SUCCESS;
}