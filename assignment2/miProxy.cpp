#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <netdb.h>

using namespace std;

void errorOut(string errorMessage) {
	cout << errorMessage << endl;
	exit(1);
}

void http_proxy(char* browser_port) {
	cout << browser_port << endl;
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	int sockfd, newfd;
	struct sockaddr_storage storage_addr;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	//get socket info
	if ((status = getaddrinfo(NULL, browser_port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error %s\n", gai_strerror(status));
		exit(1);
	}
	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	//bind
	int bindResult = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);	
	if (bindResult) {
		cout << "bind error" << endl;
	}

	//listen
	cout << "no bind error, listening" << endl;
	listen(sockfd,1);
	cout << "done listening" << endl;

	//accept
	socklen_t addr_size = sizeof storage_addr;
	newfd = accept(sockfd, (struct sockaddr *)0, &addr_size);
	cout << "accepted" << endl;

	//connect to apache
	int status_server;
	struct addrinfo hints_server;
	struct addrinfo *servinfo_server;
	int sockfd_server;

	memset(&hints_server, 0, sizeof hints_server);
	hints_server.ai_family = AF_UNSPEC;
	hints_server.ai_socktype = SOCK_STREAM;
	hints_server.ai_flags = AI_PASSIVE;

	//get socket info
	if ((status_server = getaddrinfo(NULL, to_string(80).c_str(), &hints_server, &servinfo_server)) != 0) {
		fprintf(stderr, "getaddrinfo error %s\n", gai_strerror(status_server));
		exit(1);
	}
	sockfd_server = socket(servinfo_server->ai_family, servinfo_server->ai_socktype, servinfo_server->ai_protocol);
	
	//connect
	int connect_status = connect(sockfd_server, servinfo_server->ai_addr, servinfo_server->ai_addrlen);
	if (connect_status) {
		cout << "connect error" << endl;
	}
	cout << "connected to server" << endl;

	int BUF_SIZE = 700000;
	char buf[700000] = { 0 };
	char buf2[700000] = { 0 };
	time_t start = time(NULL);
	int num_bytes;
	int num_bytes2;
	while(true) {

		cout << "block 1\n";
		num_bytes = recv(newfd, buf, BUF_SIZE, 0);
		cout << "received\n";
		send(sockfd_server, buf, num_bytes, 0);
		cout << "sent\n";
		cout << buf << endl;

		cout << "block 2\n";
		num_bytes2 = recv(sockfd_server, buf2, BUF_SIZE, 0);
		cout << "received\n";
		send(newfd, buf2, num_bytes2, 0);
		cout << "sent\n";
		cout<< buf2 << endl;

	}

	shutdown(newfd, 2);
	shutdown(sockfd_server,2);
}

int main(int argc, char* argv[]) {

	string message = "Use format: ./miProxy <log> <alpha> <listen_port> <www-ip>";

	if (argc != 5) {
		errorOut(message);
	}

	char* log = argv[1];
	float alpha = atof(argv[2]);
	char* listen_port = argv[3];
	char* www_ip = argv[4];

	http_proxy(listen_port);	
}
