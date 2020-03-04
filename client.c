#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>

int send_to_server(int network_socket) {
	char message[256] = "";

	fgets(message, 256, stdin);
	message[strlen(message) - 1] = 0;
	if (strcmp(message, "End") == 0) {
		return 0;
	}

	char message_length[256];
	sprintf(message_length, "%ld", strlen(message));

	if (send(network_socket, &message_length, strlen(message_length), 0) < 0) {
		perror("Send error");
		exit(1);
	}

	if (recv(network_socket, &message_length, 256, 0) < 0) {
		perror("Receive error");
		exit(1);
	}

	if (send(network_socket, message, strlen(message), 0) < 0) {
		perror("Send error");
		exit(1);
	}

	if (recv(network_socket, &message, 256, 0) < 0) {
		perror("Receive error");
		exit(1);
	}

	printf("%s\n", message);

	return 1;
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
	server_address.sin_addr.s_addr = inet_addr("172.20.10.13");

	//call the connect function
	int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	if (connection_status < 0) {
		perror("Connection error");
		exit(1);
	}

	//recieve data from server
	char server_response[256];
	if (recv(network_socket, &server_response, sizeof(server_response), 0) < 0) {
		perror("Receive error");
		exit(1);
	}

	//print server response
	printf("%s\n", server_response);

	//Data exchange
	while (send_to_server(network_socket));

	//close the socket
	close(network_socket);
	return 0;
}