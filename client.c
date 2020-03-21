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
#include <termios.h>

#define IP "192.168.1.5"

char username[256] = "";
char password[256] = "";
int pid;

char* readPassword(){
	struct termios oflags, nflags;
	char* password = (char*)malloc(255 * sizeof(char));

	/* disabling echo */
	tcgetattr(fileno(stdin), &oflags);
	nflags = oflags;
	nflags.c_lflag &= ~ECHO;
	nflags.c_lflag |= ECHONL;

	if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
		perror("tcsetattr");
		exit(1);
	}

	scanf("%s", password);

	/* restore terminal */
	if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
		perror("tcsetattr");
		exit(1);
	}

	return password;
}

int getUserAndPassword(int network_socket) {
	int n = 0;
	int found = 0;
	char credentials[512] = "";

	while (n < 5 && found == 0) {
		system("clear");
		//read user input 
		printf("Enter your username:\n");
		scanf("%s", username);
		printf("Enter your password:\n");
		strcpy(password,readPassword());

		sprintf(credentials, "%s:%s", username, password);

		//send the credentials length
		char credentials_length[256];
		sprintf(credentials_length, "%ld", strlen(credentials));
		if (send(network_socket, &credentials_length, sizeof(int), 0) < 0) {
			perror("Send error");
			exit(1);
		}

		//send credentials
		if (send(network_socket, credentials, strlen(credentials), 0) < 0) {
			perror("Send error");
			exit(1);
		}

		//receive 'Accepted' or 'Rejected'
		char data[256] = "";
		if (recv(network_socket, &data, 9, 0) <= 0) {
			perror("Receive error");
			exit(1);
		}

		if (strcmp(data, "Accepted") == 0) {
			found = 1;
		}

		if (!found && n<4) {
			printf("No user with such credentials exists! Retry!\n");
			getchar();
			getchar();
		}

		n++;
	}

	if (1 == found) {
		printf("Login Succesful!\n");
	}
	else {
		printf("You are out of try's!\n");
	}

	return found;
}

int sendToServer(int network_socket) {
	//read user message
	char message[256] = "";
	fgets(message, 256, stdin);
	message[strlen(message) - 1] = 0;

	//exit for blank messages
	if (strlen(message) == 0) {
		return 1;
	}

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

	if (0 == getUserAndPassword(network_socket)) {
		return 1;
	}

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
