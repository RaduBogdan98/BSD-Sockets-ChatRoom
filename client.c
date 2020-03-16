#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <inttypes.h>
#include <arpa/inet.h>

#define IP "192.168.1.5"

char username[256] = "";
int pid;

int getUserAndPassword() {
	printf("Please Login\n\n");
	printf("Username: ");
	fgets(username, 256, stdin);
	username[strlen(username) - 1] = 0;

	//Send the username to server 

	//Same thing for the password

	//The server will check if the credentials are correct and send back a response
	//Based on the response this function returns 1 or 0

	return 1;
}

int sendToServer(int network_socket) {
	//read user message
	char message[256] = "";
	fgets(message, 256, stdin);
	message[strlen(message) - 1] = 0;

	//treat end message
	if (strcmp(message, "End") == 0) {
		kill(pid, SIGINT);
		return 0;
	}

	//Add user caption to the message
	char captioned_message[256] = "";
	sprintf(captioned_message, "%s: %s", username, message);

	//send the message length
	char message_length[256];
	sprintf(message_length, "%ld", strlen(captioned_message));
	if (send(network_socket, &message_length, sizeof(int), 0) < 0) {
		perror("Send error");
		exit(1);
	}

	//send the message
	if (send(network_socket, captioned_message, strlen(captioned_message), 0) < 0) {
		perror("Send error");
		exit(1);
	}

	return 1;
}

void receiveFromServer(int network_socket) {
	int data_size = 0;
	char data[256] = "";

	//recieve message size
	if (recv(network_socket, &data, sizeof(int), 0) <= 0) {
		perror("Receive error");
		exit(1);
	}

	//convert message to integer
	data_size = atoi(data);
	bzero(data, strlen(data));

	//receive the and print the message 
	//or close the connection if it has ended
	int status = recv(network_socket, &data, data_size, 0);
	if (status < 0) {
		perror("Receive error");
		exit(1);
	}
	else if (status == 0) {
		return;
	}
	else {
		printf("%s \n", data);
	}
	bzero(data, strlen(data));

	return;
}

int main() {
	//create a socket
	int network_socket;
	network_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (network_socket < 0) {
		perror("Socket creation error");
		exit(1);
	}

	//specify an address for the socket
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9002);
	server_address.sin_addr.s_addr = INADDR_ANY;

	//call the connect function
	int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	if (connection_status < 0) {
		perror("Connection error");
		exit(1);
	}

	getUserAndPassword();

	//receive data from server
	char server_response[256];
	if (recv(network_socket, &server_response, sizeof(server_response), 0) < 0) {
		perror("Receive error");
		exit(1);
	}

	//Print server response
	printf("%s\n", server_response);

	pid = fork();
	if (pid < 0) {
		perror("Fork error");
		exit(1);
	}
	else if (pid == 0) {
		while (1) {
			receiveFromServer(network_socket);
		}
	}

	//Data exchange
	while (sendToServer(network_socket));

	//close the socket
	close(network_socket);
	return 0;
}
