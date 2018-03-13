#include <string.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <netdb.h>
#include <time.h>

using namespace std;

void error_out(string errorMessage) {
	cout << errorMessage << endl;
	exit(1);
}

int find_send_size(char* buffer){

	int size;
	string buf_str(buffer);

	int header_end = buf_str.find("\r\n\r\n");
	
	if (buf_str.find("GET") != string::npos) {
		size = header_end + 4;
	} else {
		int cont_loc = buf_str.find("Content-Length:");
		char curr_char = buf_str[cont_loc+15];
		int counter = 1;
		string size_string = "";
		while (curr_char != '\n'){
			curr_char = buf_str[cont_loc+15+counter];
			size_string += curr_char;
			counter += 1;
		}
		size = stoi(size_string) + header_end + 4;
	}
	return size;

}

float ewma_tp(float T_new, float T_curr, float alpha) {

	float throughput;

	if (T_curr == -1){
		throughput = T_new;
	} else {
		throughput = alpha * T_new + (1 - alpha) * T_curr;
	}

	return throughput;
}

string modify_get_bitrate(char* req_buffer, int new_bitrate, int old_bitrate) {
	string req_buf(req_buffer);
	int vod_loc = req_buf.find("vod/");
	char curr_char = req_buf[vod_loc+4];
	int counter = 0;
	while (curr_char != 'S') {
		counter++;
		curr_char = req_buf[vod_loc+4+counter];
	}
	req_buf.erase(vod_loc+4, counter);
	req_buf.insert(vod_loc+4, to_string(new_bitrate));
	cout << "modify get bitrate: *****************\n";
	cout << new_bitrate << endl;
	cout << req_buf << endl << req_buffer<< endl;

	return req_buf;
	

}

string get_new_request(char* req_buf){

	// find .f4m filename in response buffer
	string rb(req_buf);
	int period_loc = rb.find(".f4m");
	rb.insert(period_loc, "_nolist");
	return rb;
}



int new_send(int sockfd, char* buf, int bytes_received) {

	int bytes_sent = send(sockfd, buf, bytes_received, 0);
	int true_bytes = find_send_size(buf);
	while(bytes_sent != true_bytes) {
		bytes_sent += send(sockfd, buf + bytes_sent, true_bytes - bytes_sent, 0);
	}
	return bytes_sent;
}

int new_recv(int sockfd, char* buf, int BUF_SIZE) {

	int bytes_received = recv(sockfd, buf, BUF_SIZE, 0);
	int true_bytes = find_send_size(buf);
	while(bytes_received != true_bytes) {
		bytes_received += recv(sockfd, buf + bytes_received, BUF_SIZE - bytes_received, 0);
	}
	return bytes_received;
}
int* find_bitrates(char* buffer){
	int bits[3];
	string buf(buffer), br;
	int counter = 0;
	size_t n = buf.find("bitrate=", 0);
	while(n != string::npos) {
		counter++;
		n = buf.find("bitrate=", n+1);
		if (n > buf.length()) {
			break;
		}
		br = buf.substr(n+8,5);
		for (int i = 0; i < br.length(); i++) {
			if (br[i] == '"') {
				br.erase(i, 1);
			}
		}
		bits[counter-1] = stoi(br);
	}
	int* bitrates = bits;
	return bitrates;
}
string find_chunk(char* buffer){

	string req_buf(buffer);

	int vod_loc = req_buf.find("vod/");
	char curr_char = req_buf[vod_loc+4];
	int counter = 0;
	while (curr_char != ' ') {
		counter++;
		curr_char = req_buf[vod_loc+4+counter];
	}
	string chunk = req_buf.substr(vod_loc+4, counter);

	return chunk;

}

void http_proxy(char* browser_port, float alpha, char* log) {

	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	int sockfd, newfd;
	struct sockaddr_storage storage_addr;
	ofstream logfile;
	logfile.open(log);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	//CONNECT TO BROWSER
	//get socket info
	if ((status = getaddrinfo(NULL, browser_port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error %s\n", gai_strerror(status));
		exit(1);
	}
	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	//bind
	int bindResult = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);	
	if (bindResult) {
		error_out("bind error");
	}

	//listen
	cout << "listening\n";
	listen(sockfd,1);

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

	int BUF_SIZE = 700000;
	char buf[700000];
	char buf2[700000];
	memset(buf, 0, sizeof(buf));
	clock_t start, finish;
	int num_bytes, num_bytes2, req_size, bytes_sent;
	float tp_hold;
	float throughput = -1;
	string buf_string;

	//receive request from browser
	num_bytes = recv(newfd, buf, BUF_SIZE, 0);
	//send request to server
	bytes_sent = new_send(sockfd_server, buf, num_bytes);
	//cout << buf << endl;
	
	//receive response from server
	num_bytes2 = new_recv(sockfd_server, buf2, BUF_SIZE);
	cout << buf2 << endl;
	//on first iteration parse buffer for .f4m file and send _nolist

	//fetch big_buck_bunny.f4m for proxy
	//return an array of bitrates
	int bytes_received;
	for (int i = 0; i < 3; i++) {

		bytes_sent = new_send(newfd, buf2, num_bytes2);
		bytes_received = new_recv(newfd, buf, BUF_SIZE);
		bytes_sent = new_send(sockfd_server, buf, bytes_received);
		num_bytes2 = new_recv(sockfd_server, buf2, BUF_SIZE);
	}
	int* bitrates;
	bitrates = find_bitrates(buf2);
	int br[] = {bitrates[0], bitrates[1], bitrates[2]};
	cout << br << endl;
	

	char* new_buf;
	int curr_tp = 1000;
	int curr_tp_ind = 2;
	int next_tp = 99999;
	cout << next_tp << endl;
	string new_request = get_new_request(buf);
	req_size = new_request.length();
	new_buf = new char[req_size];
	strcpy(new_buf, new_request.c_str());
	while(true) {

		//loop through sends and receives until video is finished
		start = clock();
		bytes_sent = new_send(sockfd_server, new_buf, sizeof(new_buf));
		new_buf = new char[BUF_SIZE];
		num_bytes2 = new_recv(sockfd_server, new_buf, BUF_SIZE);
		bytes_sent = new_send(newfd, new_buf, req_size);
		finish = clock();

		//calculate throughput
		tp_hold = num_bytes2/difftime(finish,start);
		throughput = ewma_tp(tp_hold, throughput, alpha);	
		new_recv(newfd, new_buf, BUF_SIZE);

		//find name of segment
		string chunk_name = find_chunk(new_buf);

		//update log
		logfile << difftime(finish,start) << " " << tp_hold << " ";
		logfile << throughput << " " << curr_tp << " 127.0.0.1 " << chunk_name << endl;


		//check if bitrate needs to be changed and modify get requests
		if (throughput > 1.5 * next_tp) {
			curr_tp_ind++;
			curr_tp = br[curr_tp_ind];
			cout << "greater than " << curr_tp << endl;
			new_request = modify_get_bitrate(new_buf, curr_tp, br[curr_tp_ind]);
			req_size = new_request.length();
			new_buf = new char[req_size];
			strcpy(new_buf, new_request.c_str());
			cout << "modified bitrate: "<<  new_buf << endl;
			if (curr_tp_ind == 2){
				next_tp = 99999;
			} else {
				next_tp = br[curr_tp_ind+1];
			}
	
		} else if(throughput < 1.5*curr_tp && curr_tp_ind != 0) {
			curr_tp_ind--;
			curr_tp = br[curr_tp_ind];
			cout << "modifying br 1:\n";
			cout << br[0] << endl;
			cout << curr_tp << endl;
			cout << "less than" << curr_tp_ind<< endl;
			new_request = modify_get_bitrate(new_buf, curr_tp, br[curr_tp_ind]);
			cout << "modified bitrate2: " << new_buf << endl;
			req_size = new_request.length();
			new_buf = new char[req_size];
			strcpy(new_buf, new_request.c_str());
			next_tp = br[curr_tp_ind+1];
		} else{
			
			new_request = modify_get_bitrate(new_buf, curr_tp, br[curr_tp_ind]);
			cout << "modified bitrate2: " << new_buf << endl;
			req_size = new_request.length();
			new_buf = new char[req_size];
			strcpy(new_buf, new_request.c_str());
			break;
		}
		
	}
	shutdown(newfd, 2);
	shutdown(sockfd_server,2);
}

int main(int argc, char* argv[]) {

	string message = "Use format: ./miProxy <log> <alpha> <listen_port> <www-ip>";

	if (argc != 5) {
		error_out(message);
	}

	char* log = argv[1];
	float alpha = atof(argv[2]);
	char* listen_port = argv[3];
	char* www_ip = argv[4];

	http_proxy(listen_port, alpha, log);
}
