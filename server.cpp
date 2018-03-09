#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <cerrno>

using namespace std;

#define MAXLINE 1000
#define LISTENQ 100
#define BLANK ' ' || '\t'

int TOTALCLI = 0;

struct user
{
	string name;
	string addr;
	string port;
	int die;
	user(string const& str, string a, string p):name(str){
		addr = a;
		port = p;
		die = 0;
	}
	user()
	{
	}
};

struct user childarr[FD_SETSIZE];
int client[FD_SETSIZE];
fd_set readSet, allSet;

void skipblank(char *str){
	int i = 0;
	while(str[i++] == ' ');
	i--;
	int j = 0;
	while((j + i) < strlen(str)){
		str[j] = str[j + i];
		j++;
	}
	str[j] = '\0';
}

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

void tabToBlank(char *str){
	for(int i=0;i<strlen(str);i++){
		if(str[i] == '\t')
			str[i] = ' ';
	}
}

int getIndex(const string msg, struct user *arr){
	char addr[20];
	char port[20];
	int i = 0;
	while(msg[i]!=' '){
		addr[i] = msg[i];
		i++;
	}
	i++;	// so i will not be ' '
	int j = 0;
	while(msg[j + i]!=','){
		port[j] = msg[j + i];
		j++;
	}
	addr[i-1] = '\0';
	port[j] = '\0';
	for(i=0;i<FD_SETSIZE;i++){
		if(arr[i].addr == string(addr) && arr[i].port == string(port))
			return i;
	}
	return -1;
}

int changeName(int index, string name){
	int j=0;
	string temp = "";
	while(name[j] != ' '){
		temp += name[j++];
		if(j == name.size())
			break;	// end
	}
	cout << j << ", " << name.size();
	if(j < name.size()){
		// get ' '
		while(name[j++] == ' ');
		cout << j << ", " << name.size();
		if(j == name.size() + 1){
			cout << ".............\n";
			cout << name << endl;
			name = temp;
			cout << name << endl;
		}
		else
			return 3;	//	name not alphabets
	}

	cout << name.size() << ",, " << name << endl;
	if(name.size() > 12 || name.size() < 2)
		return 0;	//	name size error
	if(name == "anonymous\0")
		return 1;	//	name = anonymous
	for(int i=0;i<FD_SETSIZE;i++){
		if(name == childarr[i].name && client[i] > 0)
			return 2;	//	name already exist
	}
	for(int i=0;i<name.size();i++){
		if(!((name[i] <= 'z' && name[i] >= 'a') || (name[i] <= 'Z' && name[i] >= 'A')))
      		return 3;	//	name not alphabets
	}

	//	name permitted
	childarr[index].name = name;
	return 4;
}

void sendAll(string msg, int except){
	for(int i=0;i<FD_SETSIZE;i++){
		if(childarr[i].die == 0 && i != except)
			write(client[i], msg.c_str(), msg.size());
	}
}

void dealString(string msg){
	int index = getIndex(msg, childarr);
	char recvline[msg.size()];
	int i=0;
	while(msg[i++]!=',');
	//	ignore addr and port
	int j;
	for(j=i;j<msg.size();j++)
		recvline[j-i] = msg[j];
	recvline[j-i] = '\0';


		tabToBlank(recvline);
		skipblank(recvline);
		cout << string(recvline) << endl;
		
		if(recvline[string(recvline).size()-1] == '\n' && recvline[string(recvline).size()-2] == '\r')
			recvline[string(recvline).size()-2] = '\0';
		
		if(recvline[0] == '\x03'){
			cout << "Client terminates...\n";
			string msg = "[Server] " + childarr[index].name + " is offline.\n";
			sendAll(msg, index);
			close(client[index]);
			client[index] = -1;
			TOTALCLI--;
			FD_CLR(client[index], &allSet);
		}


		if(strlen(recvline) < 2){
			string msg = "ERROR: Error command.";
			write(client[index], msg.c_str(), msg.size());
			return;
		}
		else{
			char command[5];
			for(int j=0;j<4;j++)
				command[j] = recvline[j];
			command[4] = '\0';
			if(strcmp(command, "exit\0") == 0 || strcmp(command, "exit ") == 0){
				childarr[index].die = 1;	//	mark with die
				
				cout << "Client terminates...\n";
				string msg = "[Server] " + childarr[index].name + " is offline.\n";
				sendAll(msg, index);
				close(client[index]);
				client[index] = -1;
				TOTALCLI--;
				FD_CLR(client[index], &allSet);
				return;
			}
		}

		char temp[5];
		for(int i=0;i<4;i++)
			temp[i] = recvline[i];
		temp[i] = '\0';
		if(strcmp(temp, "who \0") == 0 || strcmp(recvline, "who\0") == 0 ){
		//	list all online users
			string msg = "";
			for(int j=0;j<FD_SETSIZE;j++){
				if(client[j] > 0){
					if(index != j){
						msg += "[Server] " + childarr[j].name + " " + childarr[j].addr + "/" + childarr[j].port + "\n";
					}
					else{
						msg += "[Server] " + childarr[j].name + " " + childarr[j].addr + "/" + childarr[j].port + " ->me\n";
					}
				}
			}
			msg += "\0";
			write(client[index], msg.c_str(), msg.size());
		}
		else{
			
			//	*********************************************
			char command[6];
			for(int j=0;j<5;j++)
				command[j] = recvline[j];
			command[5] = '\0';
			//	*********************************************
			if(strcmp(command, "name \0") == 0){
				string oldName = childarr[index].name;
				for(i=0;i<string(recvline).size()-5;i++)
					recvline[i] = recvline[i + 5];
				recvline[i] = '\0';

				skipblank(recvline);

				int status = changeName(index, recvline);
				switch(status){
					case 0:{
						string msg = "[Server] ERROR: Username can only consists of 2~12 English letters.\n";
						write(client[index], msg.c_str(), msg.size());
						break;
					}
					case 1:{
						string msg = "[Server] ERROR: Username cannot be anonymous.\n";
						write(client[index], msg.c_str(), msg.size());
						break;
					}
					case 2:{
						string msg = "[Server] ERROR: " + string(recvline) + " has been used by others.\n";
						write(client[index], msg.c_str(), msg.size());
						break;
					}
					case 3:{
						string msg = "[Server] ERROR: Username can only consists of 2~12 English letters.\n";
						write(client[index], msg.c_str(), msg.size());
						break;
					}
					case 4:{
						for(int j=0;j<FD_SETSIZE;j++){
							if(childarr[j].die == 0){
								if(j == index){
									string msg = "[Server] You're now known as " + string(recvline) + ".\n";
									write(client[j], msg.c_str(), msg.size());
								}
								else{
									string msg = "[Server] " + oldName + " is now known as " + string(recvline) + ".\n";
									write(client[j], msg.c_str(), msg.size());
								}	
							}
						}
						break;
					}
					default:{
						break;
					}
				}
			}
			else if(strcmp(command, "tell \0") == 0){
				for(i=0;i<string(recvline).size()-5;i++)
					recvline[i] = recvline[i + 5];
				recvline[i] = '\0';
				string name = "", msgToSend = "";
				

				skipblank(recvline);

				int i = 0;
				while(recvline[i] != ' ' && recvline[i] != '\n' && recvline[i] != '\0')
					name += recvline[i++];
				i++;

				while(i < strlen(recvline))
					msgToSend += recvline[i++];
				
				if(msgToSend == ""){
					string msg = "ERROR: Error command.\n";
					write(client[index], msg.c_str(), msg.size());
					return;
				}

				
				skipblank(msgToSend);

				int selfNameError = 0, error = 0, toWho;
				string msg = "";
				if(childarr[index].name == "anonymous"){
					selfNameError = 1;
					error = 1;
					msg += "[Server] ERROR: You are anonymous.";
				}
				if(name == "anonymous"){
					error = 1;
					if(selfNameError == 1)
						msg += "\n";
					msg += "[Server] ERROR: The client to which you sent is anonymous.";
				}
				else{
					int j;
					for(j=0;j<FD_SETSIZE;j++){
						if(name == childarr[j].name && client[j] > 0)	//	name exist and still alive
							break;
					}
					
					if(j == FD_SETSIZE){
						error = 1;
						if(selfNameError == 1)	// have msg already
							msg += "\n";
						msg += "[Server] ERROR: The receiver doesn't exist.";	
					}
					else
						toWho = j;
				}
				if(error == 1){
					msg += "\n";
					write(client[index], msg.c_str(), msg.size());
				}
				else{
					//	no error
					string msg = "[Server] SUCCESS: Your message has been sent.\n";
					write(client[index], msg.c_str(), msg.size());
					msg = "[Server] " + childarr[index].name + " tell you " + msgToSend + "\n";
					write(client[toWho], msg.c_str(), msg.size());
				}
			}
			else if(strcmp(command, "yell \0") == 0){
				for(i=0;i<string(recvline).size()-5;i++)
					recvline[i] = recvline[i + 5];
				recvline[i] = '\0';

				skipblank(recvline);
				if(strcmp(recvline, "") == 0){
					string msg = "ERROR: Error command.\n";
					write(client[index], msg.c_str(), msg.size());
					return;
				}



				string msg = "[Server] " + childarr[index].name + " yell " + recvline + "\n";
				for(int j=0;j<FD_SETSIZE;j++){
					if(childarr[j].die == 0)
						write(client[j], msg.c_str(), msg.size());
				}
			}
			else{
				string msg = "ERROR: Error command.\n";
				write(client[index], msg.c_str(), msg.size());
			}
		}

}

int main(int argc, char ** argv)
{	
	if(argc < 2){
		cerr << "command error...\n";
		exit(0);
	}

	int maxi, maxfd, listenfd, connectfd, sockfd;
	int nready;


	socklen_t clientLen;

	struct sockaddr_in cliaddr, seraddr;


	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&seraddr, sizeof(seraddr));
	seraddr.sin_family = AF_INET;
	seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	seraddr.sin_port = htons(atoi(argv[1]));

	bind(listenfd, (sockaddr *) &seraddr, sizeof(seraddr));
	listen(listenfd, LISTENQ);

	maxfd = listenfd;
	maxi = -1;
	for(int i=0;i<FD_SETSIZE;i++)
		client[i] = -1;	// initialize with no one in client array
	FD_ZERO(&allSet);	// initialize
	FD_SET(listenfd, &allSet);	//	listen to listenfd

	while(1){
		readSet = allSet;
		nready = select(maxfd + 1, &readSet, NULL, NULL, NULL);

		if(FD_ISSET(listenfd, &readSet)){
			clientLen = sizeof(cliaddr);
			connectfd = accept(listenfd, (sockaddr *) &cliaddr, &clientLen);

			int i;
			for(i=0;i<FD_SETSIZE;i++){
				if(client[i] < 0){
					//	client in
					client[i] = connectfd;
					cout << "client: " << client[i] << endl;
					char temp[5];
					sprintf(temp,"%4d",(int)cliaddr.sin_port);
					string msg = string("[Server] Hello, anonymous! From: " + string(inet_ntoa(cliaddr.sin_addr)) + '/' + temp + '\n');
					string msg2 = "[Server] Someone is coming!\n";

					childarr[i] = user("anonymous", string(inet_ntoa(cliaddr.sin_addr)), temp);
					
					write(connectfd, msg.c_str(), msg.size());	//	send welcoming msg to new client
					sendAll(msg2, i);

					TOTALCLI++;
					break;
				}
			}
			if(i == FD_SETSIZE){
				cout << "Too many client...\n";
				exit(0);
			}
			
			FD_SET(connectfd, &allSet);
			if(connectfd > maxfd)
				maxfd = connectfd;
			if(maxi < i)
				maxi = i;
			if(--nready <= 0)
				continue;
		}

		for(int i=0;i<=maxi;i++){
			if((sockfd = client[i]) < 0)
				continue;
			if(FD_ISSET(sockfd, &readSet)){
				int n;
				char buffer[MAXLINE];
				bzero(buffer, MAXLINE);
				if((n = recv(sockfd, buffer, MAXLINE, 0)) > 0){
					buffer[n] = 0;
					string msg = childarr[i].addr + " " + childarr[i].port + "," + string(buffer);
					
					dealString(msg);
				}
				if(n == 0){
					cout << "Client terminates...\n";
					string msg = "[Server] " + childarr[i].name + " is offline.\n";
					sendAll(msg, i);
					close(sockfd);
					TOTALCLI--;
					FD_CLR(sockfd, &allSet);
					client[i] = -1;
				}
				if(--nready <= 0)
					break;	// no more readable descriptors
			}
		}


	}


	return 0;
}