#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <netdb.h>

using namespace std;

void errorOut(string errorMessage) {
	cout << errorMessage << endl;
	exit(1);
}


int main(int argc, char* argv[]) {

	message = "Use format: ./miProxy <log> <alpha> <listen_port> <www-ip>";

	if (argc != 5) {
		errorOut(message);
	}

	char* log = argv[1];
	float alpha = atof(argv[2]);
	char* listen_port = argv[3];
	char* www_ip = argv[4];

	
}
