#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#define IP "192.168.1.160"

void process(int client_socket) {
	int data_size = 0;
	char data[256] = "";

	//recieve message size
	if (recv(client_socket, &data, sizeof(int), 0) < 0) {
		perror("Receive error");
		exit(1);
	}
	//convert message to integer
	data_size = atoi(data);

	//send response
	if (send(client_socket, "Data received", 256, 0) < 0) {
		perror("Receive error");
		exit(1);
	}

	//receive the and print the message 
	//or close the connection if it has ended
	int status = recv(client_socket, &data, data_size, 0);
	if (status < 0) {
		perror("Receive error");
		exit(1);
	}
	else if (status == 0) {
		return;
	}
	else {
		printf("%s \n",data);
	}

	//send process end message
	char server_message[256] = "Request processed";
	if (send(client_socket, server_message, sizeof(server_message), 0) < 0) {
		perror("Receive error");
		exit(1);
	}

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
		//Accept clients
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
