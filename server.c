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
#include <time.h>

#define IP "192.168.1.5"

int sockets[20];
int clientsCount;
int pfd[2];
char username[256];

int checkCredentials(int client_socket) {
	int n = 0;
	int found = 0;

	while (n < 5 && found == 0) {
		char credentials[256] = "";
		int dataSize = 0;

		//receive credentials length
		int status = recv(client_socket, &credentials, sizeof(int), 0);
		if (status < 0) {
			perror("Receive error");
			exit(1);
		}
		else if (status == 0) {
			return 0;
		}

		dataSize = atoi(credentials);

		//receive credentials
		status = recv(client_socket, &credentials, dataSize, 0);
		if (status < 0) {
			perror("Receive error");
			exit(1);
		}
		else if (status == 0) {
			return 0;
		}

		//try to find credentials
		FILE* filePointer = fopen("userData.txt", "r");
		char buffer[255];
		while (fgets(buffer, 255, filePointer)) {
			int len = strlen(buffer);
			if (len > 0 && buffer[len - 1] == '\n') {
				buffer[len - 1] = 0;
			}

			if (strcmp(credentials, buffer) == 0) {
				found = 1;
			}
		}

		//send response
		if (1 == found) {
			if (send(client_socket, "Accepted", strlen("Accepted"), 0) < 0) {
				perror("Send error");
				exit(1);
			}
			char* p = strtok(credentials, ":");
			strcpy(username,p);

			return 1;
		}
		else {
			if (send(client_socket, "Rejected", strlen("Rejected"), 0) < 0) {
				perror("Send error");
				exit(1);
			}
		}

		fclose(filePointer);
		n++;
	}

	return 0;
}

//remove a socket form the sockets list
void removeSocket(int socketFd) {
	int i;
	for (i = 0; i < clientsCount; i++) {
		if (sockets[i] == socketFd) break;
	}

	close(sockets[i]);

	for (int j = i; j < clientsCount - 1; j++) {
		sockets[j] = sockets[j + 1];
	}
	clientsCount--;
}

void sendToAll(char* buff) {
	int socketsToRemove[clientsCount];
	int count = 0;

	for (int i = 0; i < clientsCount; i++) {
		char length[256] = "";
		sprintf(length, "%ld", strlen(buff));

		if (send(sockets[i], length, sizeof(int), 0) <= 0) {
			//if the pipe is broken the client has closed the pipe AKA closed the connection
			if (errno == EPIPE) {
				//mark the socket to be removed
				socketsToRemove[count] = sockets[i];
				count++;
				continue;
			}
			else {
				perror("Send error");
				exit(1);
			}
		}

		if (send(sockets[i], buff, strlen(buff), 0) <= 0) {
			if (errno == EPIPE) {
				socketsToRemove[count] = sockets[i];
				count++;
				continue;
			}
			else {
				perror("Send error");
				exit(1);
			}
		}
	}

	//remove the broken sockets
	for (int i = 0; i < count; i++) {
		removeSocket(socketsToRemove[i]);
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
	int status = recv(client_socket, &data, sizeof(int), 0);
	if (status < 0) {
		perror("Receive error");
		exit(1);
	}
	else if (status == 0) {
		printf("%s has left the server, from socket %d!\n", username, client_socket);
		exit(0);
	}
	write(pfd[1], &data, sizeof(int));

	//convert message to integer
	data_size = atoi(data);
	bzero(data, strlen(data));

	//receive the and print the message 
	//or close the connection if it has ended
	status = recv(client_socket, &data, data_size, 0);
	if (status < 0) {
		perror("Receive error");
		exit(1);
	}
	else if (status == 0) {
		printf("%s has left the server, from socket %d!\n", username, client_socket);
		exit(0);
	}
	else {
		time_t my_time;
		struct tm * timeinfo;
		time(&my_time);
		timeinfo = localtime(&my_time);

		printf("%d:%d:%d - Message send by %s!\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, username);
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

	struct sigaction interruptAction;
	interruptAction.sa_handler = SIG_IGN;
	interruptAction.sa_flags = 0 | SA_RESTART;

	if (sigaction(SIGPIPE, &interruptAction, NULL) < 0)
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
	server_address.sin_addr.s_addr = INADDR_ANY;
	//inet_addr(IP)

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
		if (0 == checkCredentials(client_socket)) {
			continue;
		}

		printf("%s has connected to the server on socket %d!\n", username, client_socket);
		sockets[clientsCount] = client_socket;
		clientsCount++;

		if (fork() == 0) {
			close(pfd[0]);
			close(server_socket);
			send(client_socket, server_message, sizeof(server_message), 0);

			sendAction.sa_handler = SIG_IGN;
			sendAction.sa_flags = 0 | SA_RESTART;

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

	for (int i = 0; i < clientsCount; i++) {
		close(sockets[i]);
	}
	close(server_socket);
	return 0;
}
