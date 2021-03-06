#include "client.h"
#include <iostream>
#include <string>

using namespace std;

Client::Client(int port) {
	on = false;
	ip = "127.0.0.1";
	port_c = port;
	port_b = port + 1;

	socket_c = socket(AF_INET, SOCK_DGRAM, 0);
	socket_b = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_c < 0 || socket_b < 0) {
		printf("fail to create sockets!\n");
		exit(-1);
	}

	int yes = 1;
	if (setsockopt(socket_c, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {  
        perror("setsockopt");  
        exit(-1);  
    }  

    if (setsockopt(socket_b, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {  
        perror("setsockopt");  
        exit(1);  
    }  

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(8000);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	bzero(&test_beat, sizeof(test_beat));
	test_beat.sin_family = AF_INET;
	test_beat.sin_port = htons(9000);
	test_beat.sin_addr.s_addr = inet_addr("127.0.0.1");
}

Client::~Client() {}

void Client::bind_address() {
	bzero(&client, sizeof(client));
	client.sin_family = AF_INET;
	client.sin_port = htons(port_c);
	client.sin_addr.s_addr = inet_addr("127.0.0.1");

	bzero(&beat, sizeof(beat));
	beat.sin_family = AF_INET;
	beat.sin_port = htons(port_b);
	beat.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(socket_c, (sockaddr*)&client, sizeof(client)) < 0) {
		printf("fail to bind\n");
		exit(-1);
	}

	if (bind(socket_b, (sockaddr*)&beat, sizeof(beat)) < 0) {
		printf("fail to bind\n");
		exit(-1);
	}
}

void Client::print_command() {
	cout << "Supported commands are as follow:" << endl;
	cout << "chat <name> <message> (send the message to the user named 'name')" << endl;
	cout << "help                  (show the sopported commands)" << endl;
	cout << "user                  (show the online users)" << endl;
	cout << "quit                  (exit the system)" << endl;
	cout << endl;
}

void Client::print_user() {
	if (user_list.begin() == user_list.end()) {
		cout << "No other users are online" << endl;
		return;
	}
	cout << "Online users are as follow: <name> <ip> <port>" << endl;
	for (unordered_map<string, user>::iterator i = user_list.begin(); i != user_list.end(); i++)
		cout << i->first << " " << i->second.ip << " " << i->second.port << endl;
	cout << endl;
}

void* Client::send_beat(void* arguments) {
	Client* self = (Client*) arguments;
	while (1) {
		if (self->on) {
			sleep(5);
			string message = self->name + " " + self->ip + " " + to_string(self->port_c) + "\n";
			sendto(self->socket_b, message.c_str(), message.length(), 0, (sockaddr*)&(self->test_beat), sizeof(sockaddr_in));
		} else
			break;
	}

	return 0;
}

void* Client::receive(void* arguments) {
	Client* self = (Client*) arguments;
	char buffer[4096];
	stringstream stream;
	string last = "";
	socklen_t length = sizeof(self->client);
	while (1) {
		if (self->on) {
			int value = recvfrom(self->socket_c, &buffer, 4096, 0, (sockaddr*)&(self->client), &length);
			if (value < 0) 
				continue;
			else {
				int b = 0;
				for (int index = 0; index < value; index++) {
					if (buffer[index] == '\n') {
						memset(buffer + index, '\0', 1);
						stream << last << buffer + b;
						last = "";
						string what, who;
						stream >> what >> who;
						if (what == "" || who == "") {
							b = index + 1;
							continue;
						} else if (what == "chat") {
							string mail = stream.str().substr(6 + who.length());
							cout << "message from " << who << ":" << mail << endl;
						} else if (what == "on") {
							user new_user;
							stream >> new_user.ip >> new_user.port;
							self->user_list[who] = new_user;
							cout << "User named " << who << " has logged in." << endl;
						} else if (what == "offline") {
							cout << "User named " << who << " has logged out." << endl;
							self->user_list.erase(who);
						}
						b = index + 1;
						stream.clear();
						stream.str("");
					}
				}
				if (buffer[value - 1] != '\0')
					last = buffer + b;
			}
		}
	}
}

void Client::quit() {
	cout << "releasing resources, please wait..." << endl;
	on = false;
	pthread_join(beat_p, NULL);
	pthread_join(receive_p, NULL);
	string message = "bye " + name + "\n";
	sendto(socket_c, message.c_str(), message.length(), 0, (sockaddr*)&server, sizeof(sockaddr_in));
	cout << "bye..." << endl;
}

void Client::run() {
	cout << "Welcome, please input your nickname first!" << endl;
	print_command();
	while (1) {
		cin >> name;
		string message = "login " + name + " " + ip + " " + to_string(port_c) + "\n";
		sendto(socket_c, message.c_str(), message.length(), 0, (sockaddr*)&server, sizeof(sockaddr_in));
		char buffer[4096];
		socklen_t length = sizeof(server);
		int value = recvfrom(socket_c, &buffer, 4096, 0, (sockaddr*)&client, &length);
		if (value < 0)
			cout << "Fail to connect to server, please input your nickname and try again!" << endl;
		else {
			memset(buffer + value, '\0', 1);
			stringstream stream;
			stream << buffer;
			string response;
			stream >> response;
			if (response == "ok") {
				string user_name, user_ip;
				int user_port;
				while (stream >> user_name >> user_ip >> user_port) {
					user new_user;
					new_user.ip = user_ip;
					new_user.port = user_port;
					user_list[user_name] = new_user;
				}
				cout << "Sign in successfully!" << endl;
				print_user();
				on = true;
				break;
			} else if (response == "no")
				cout << "The name has been used by others, choose another name and try again!" << endl;
		}
	}

	if (pthread_create(&beat_p, NULL, send_beat, this) != 0) {
		printf("fail to create thread\n");
		exit(-1);
	}

	if (pthread_create(&receive_p, NULL, receive, this) != 0) {
		printf("fail to create thread\n");
		exit(-1);
	}

	char commands[1001];
	while (cin.getline(commands, 1000)) {
		// cout << command <<endl;
		stringstream stream;
		stream << commands;
		string command;
		stream >> command;
		if (command == "chat") {
			string who;
			stream >> who;
			sockaddr_in destination;
			bzero(&destination, sizeof(destination));
			destination.sin_family = AF_INET;
			destination.sin_port = htons(user_list[who].port);
			destination.sin_addr.s_addr = inet_addr(user_list[who].ip.c_str());
			string mail = stream.str().substr(5 + who.length());
			string message = "chat " + name + mail + "\n";
			sendto(socket_c, message.c_str(), message.length(), 0, (sockaddr*)&destination, sizeof(sockaddr_in));
			cout << "send to " << who << ":" << mail << endl;
		} else if (command == "help")
			print_command();
		else if (command == "user")
			print_user();
		else if (command == "quit") {
			quit();
			break;
		} else if (command.length() > 0)
			cout << "Error, undefined command!" << endl;
	}
}
