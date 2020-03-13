#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <errno.h>

#define IP "192.168.1.160"

int sockets[20];
int num_clients;
int pfd[2];

int fd_is_valid(int fd)
{
	return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

void sendToAll(char* buff) {
	for (int i = 0; i < num_clients; i++) {
		if (fd_is_valid(sockets[i])) {
			char length[256]="5";
			sprintf(length, "%ld", strlen(buff));
			if (send(sockets[i], length, sizeof(int), 0) <= 0) {
				perror("Send error");
				exit(1);
			}

			printf("Am trecut\n");

			if (send(sockets[i], buff, strlen(buff), 0) <= 0) {
				perror("Send error");
				exit(1);
			}
		}
	}
}

void handler(int sig) {
	char buff[256];
	int buffSize = 0;

	if (read(pfd[0], &buff, sizeof(int)) < 0) {
		perror("Read size");
		exit(1);
	}
	buffSize = atoi(buff);

	if (read(pfd[0], &buff, buffSize) < 0) {
		perror("Read message");
		exit(1);
	}
	buff[buffSize] = 0;

	sendToAll(buff);
}

void process(int client_socket) {
	int data_size = 0;
	char data[256] = "";

	//recieve message size
	if (recv(client_socket, &data, sizeof(int), 0) < 0) {
		perror("Receive error");
		exit(1);
	}
	write(pfd[1], &data, sizeof(int));

	//convert message to integer
	data_size = atoi(data);
	bzero(data, strlen(data));

	//receive the and print the message 
	//or close the connection if it has ended
	int status = recv(client_socket, &data, data_size, 0);
	if (status < 0) {
		perror("Receive error");
		exit(1);
	}
	else if (status == 0) {
		printf("Client with socket %d has left the server!\n", client_socket);
		exit(0);
	}
	else {
		printf("%s \n", data);
	}

	//send the message to all clients
	write(pfd[1], &data, data_size);

	kill(getppid(), SIGUSR1);
	bzero(data, strlen(data));

	return;
}

int main() {
	struct sigaction sendAction;
	sendAction.sa_handler = handler;
	sendAction.sa_flags = 0 | SA_RESTART;

	if (sigaction(SIGUSR1, &sendAction, NULL) < 0)
	{
		printf("Sigaction error\n");
		return 0;
	}

	if (pipe(pfd) < 0)
	{
		perror("Pipe");
		exit(1);
	}

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
	if (listen(server_socket, 20000) < 0) {
		perror("Listen error");
		exit(1);
	}

	for (;;) {
		//Accept clients
		int client_socket;
	again:
		client_socket = accept(server_socket, NULL, NULL);
		if (client_socket < 0) {
			if (errno == EINTR) goto again;
			perror("Accept error");
			exit(1);
		}
		printf("New Client has connected to the server\n");

		sockets[num_clients] = client_socket;
		num_clients++;

		if (fork() == 0) {
			close(pfd[0]);
			close(server_socket);
			send(client_socket, server_message, sizeof(server_message), 0);

			sendAction.sa_handler = SIG_IGN;
			sendAction.sa_flags = 0;

			if (sigaction(SIGUSR1, &sendAction, NULL) < 0)
			{
				printf("Sigaction error\n");
				return 0;
			}

			//Data exchange
			for (;;) {
				process(client_socket);
			}

			exit(0);
		}
	}

	for (int i = 0; i < num_clients; i++) {
		close(sockets[i]);
	}
	close(server_socket);
	return 0;
}
