#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#define IP "192.168.0.106"

void process(int client_socket) {
	int data_size = 0, username_length = 0;
	char data[256] = "";
	char username[256] = "";

	if (recv(client_socket, &data, sizeof(int), 0) < 0) {
		perror("Receive error");
		exit(1);
	}

	if (recv(client_socket, &username_length, sizeof(int), 0) < 0) {
		perror("Receive error");
		exit(1);
	}

	if (recv(client_socket, &username, username_length, 0) < 0) {
		perror("Receive error");
		exit(1);
	}

	data_size = atoi(data);

	send(client_socket, &data_size, sizeof(data_size), 0);

	strcpy(data, "");
	int length = recv(client_socket, &data, data_size, 0);
	if (length < 0) {
		perror("Receive error");
		exit(1);
	}
	else if (length == 0) {
		return;
	}
	else {
		printf("%s: %s\n", username, data);
	}

	//send the message
	char server_message[256] = "Request processed";
	send(client_socket, server_message, sizeof(server_message), 0);

	return;
}

int main() {
	char server_message[256] = "Server reached";

	//create the server socket
	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (server_socket < 0) {
		perror("Socket creation error");
		exit(1);
	}

	//define the server address
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9002);
	server_address.sin_addr.s_addr = inet_addr(IP);

	//bind the socket to our specified IP and port
	if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
		perror("Bind error");
		exit(1);
	}

	//listen for connections
	if (listen(server_socket, 5) < 0) {
		perror("Listen error");
		exit(1);
	}

	for (;;) {
		//accept clients
		int client_socket;
		client_socket = accept(server_socket, NULL, NULL);
		if (client_socket < 0) {
			perror("Accept error");
			exit(1);
		}

		if (fork() == 0) {
			close(server_socket);
			send(client_socket, server_message, sizeof(server_message), 0);

			//Data exchange
			for (;;) {
				process(client_socket);
			}

			exit(0);
		}

		close(client_socket);
	}

	close(server_socket);
	return 0;
}
