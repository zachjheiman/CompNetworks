#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

void errorOut(string errorMessage) {

	cout << errorMessage << endl;
	exit(1);

}

bool isNumber(char* input) {
	char *endptr;
	strtol(input, &endptr, 10);
	return *endptr == '\0';
}

void clientStartup(char* port, char* hostname, char* maxTime ) {

	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	int sockfd;

	int endTime = strtol(maxTime, NULL, 10);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "gettaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	int connectStatus = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
	if (connectStatus) {
		cout << "connect error" << endl;
	}
	time_t timer;
	time_t start = time(&timer);
	char data[1000] = { 0 };
	int counter = 0;
	while (difftime(timer, start) < endTime) {
		send(sockfd, data, 1000, 0);
		time(&timer);
		counter += 1;
	}
	char ones[1000] = { 1 };
	send(sockfd, ones, 1000, 0);
	counter += 1;
	char * buf[1000] = { 0 };

	recv(sockfd, buf, 1000, 0);
	time_t end = time(NULL);
	int speed = counter / (125 * difftime(end,start));
	cout << counter << " KB sent at " << speed << " Mbps\n";
}

void serverStartup(char* port) {


	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	int sockfd, newfd;
	struct sockaddr_storage storage_addr;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) !=0) {
		fprintf(stderr, "getaddrinfo error %s\n", gai_strerror(status));
		exit(1);
	}
	sockfd = socket(servinfo->ai_family, servinfo-> ai_socktype, servinfo->ai_protocol);

	int bindResult = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
	if (bindResult) {
		cout << "bind error" << endl;
	}
	listen(sockfd, 1);

	socklen_t addr_size = sizeof storage_addr;
	newfd = accept(sockfd, (struct sockaddr *)&storage_addr, &addr_size);

	char * buf[1000] = { 0 };
	
	time_t start = time(NULL);

	int indicator = 0;
	int counter = 0;
	int recvStatus;
	while(!indicator) {
		recvStatus = recv(newfd, buf, 1000, 0);
		if (!recvStatus) {
			cout << "connection stopped\n";
			indicator = 1;
		}
		if (buf[0] != 0){
			indicator = 1;
		}
		counter += 1;
	}
	time_t end = time(NULL);
	int speed = counter / (difftime(end, start)*125);
	cout << counter << " KB read in at " << speed << " Mbps\n";

	char data[1000] = { 1 };

	send(newfd, data, 1000, 0);
}


int main(int argc, char* argv[]) {

	bool modeIsServer = true;
	int port;
	if (argc < 2) {
		errorOut("-s or -c argument required to select mode");
	}


	if (!strcmp("-s", argv[1])) {

		if (argc < 4) {
			errorOut("-p argument required to select listen port");
		}
		if (!strcmp("-p", argv[2])) {
			if (isNumber(argv[3]) ) {
				port = strtol(argv[3], nullptr, 10);
				if (port < 1024 || port > 65535) {
					errorOut("Port number must be between 1024 and 65535");
				}
			} else {
				errorOut("-p argument must be an integer");
			}
		cout << "Successfully entered server mode\n";
		serverStartup(argv[3]);
		}

	} else if (!strcmp("-c", argv[1])) {

		modeIsServer = false;
		if (argc != 8) {
			errorOut("Client mode requires -h, -p, and -t arguments");
		}
		if (strcmp(argv[2], "-h") || strcmp(argv[4], "-p") || strcmp(argv[6],"-t")) {
			errorOut("-h, -p, and -t arguments must appear in exact order");
		}
		if (!isNumber(argv[5]) || !isNumber(argv[7])) {
			errorOut("-p and -t arguments must be numeric");
		}
		port = strtol(argv[5], nullptr, 10);
		if (port < 1024 || port > 65535) {

		}
		cout << "Successfully entered client mode\n";
		clientStartup(argv[5], argv[3], argv[7]);
	} else {

		errorOut("First argument must be '-s' or '-c'");

	}



}

