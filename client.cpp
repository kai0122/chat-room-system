#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <wait.h>

using namespace std;

#define MAXLINE 3000

void skipblank(string &str){
	int i = 0;
	while(str[i++] == ' ');
	i--;
	int j = 0;
	while((j + i) < str.size()){
		str[j] = str[j + i];
		j++;
	}
	str[j] = '\0';
}

int main(int argc, char **argv)
{
	int sockfd;
	char recvline[MAXLINE];
	struct sockaddr_in servaddr;	// sockaddr_in is in <netinet/in.h>

	if(argc != 3)
		cerr << "argc error.....\n";
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		cerr << "socket error.....\n";

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));

	if(argv[1][0] <= '9' && argv[1][0] >= '0'){
		if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
			cerr << "inethtop error for " << argv[1] << ".....\n";	
	}
	
	if(connect(sockfd, (sockaddr *) &servaddr, sizeof(servaddr)))
		cerr << "connect error.....\n";


	int readpid;
	readpid = fork();
	if(readpid == 0){
		//	child
		int n;
		while(1){
			int n;
			char recvline[MAXLINE] = {};
			while((n = read(sockfd, recvline, MAXLINE)) > 0){
				recvline[n] = 0;
				cout << string(recvline);
			}
			if(n < 0){
				cerr << "read error.....\n";	
				exit(0);
			}
			if(n == 0){
				cerr << "server terminates.....\n";
				exit(0);
			}
		}	
	}
	else{
		//	parent
		while(1){
			string buff;
			getline(cin, buff);
			skipblank(buff);
			if(buff == "exit\0"){
				//	terminate connection
				//cout << "---------------------Close------------------------\n";
				shutdown(sockfd, 2);
				kill(readpid, SIGKILL);
				exit(0);
			}
			else{
				int n = 0;
		      	if( n = (send(sockfd, buff.c_str(), buff.size(), MSG_CONFIRM) < 0 )) {
		       		cout << "Client write error: " << n << endl;
		        	exit(0);
		      	}	
			}
		}
		wait(NULL);
	}

	return 0;
}