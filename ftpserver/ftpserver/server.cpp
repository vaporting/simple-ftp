#include "server.h"
#include <string>
#include <fstream>
#include <iostream>

Server::Server(){
	this->ListenSock = INVALID_SOCKET;
	this->ClientSock = INVALID_SOCKET;
	this->commSock = INVALID_SOCKET;
	this->mode = Mode::normal;
	this->root_dir = "ftp/";
	this->fileSize = 0;
}

Server::~Server(){

}

bool Server::initial(){
	struct addrinfo hints;
	struct addrinfo *result = NULL;
	//initial winsock
	int iresult = WSAStartup(MAKEWORD(2, 2), &(this->wsaData));
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//Resolve the server address and port
	iresult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);

	// Create a SOCKET for connecting to server
	ListenSock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSock == INVALID_SOCKET) {
		std::cout << "socket failed with error : " << WSAGetLastError() << std::endl;
		WSACleanup();
		return false;
	}
	
	iresult = bind(ListenSock, result->ai_addr, (int)result->ai_addrlen);
	freeaddrinfo(result);
}

void Server::run(){
	int iresult;
	iresult = listen(ListenSock, 1); // connect amount: 1
	ClientSock = accept(ListenSock, NULL, NULL);
	if (ClientSock == INVALID_SOCKET) {
		std::cout << "socket failed with error : " << WSAGetLastError() << std::endl;
		closesocket(ListenSock);
		WSACleanup();
		return ;
	}
	this->communication();

}

void Server::communication(){
	int result;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	do{
		result = recv(ClientSock, recvbuf, recvbuflen, 0);
		if (result > 0){ //receive buf
			this->processCommand(recvbuf, result);
		}
		else if (result == 0){
			std::cout << "client closing\n";
		}
		else{
			printf("recv failed with error: %d\n", WSAGetLastError());
		}
	} while (result > 0);
}

void Server::processCommand(char* buf, int len){
	std::string command = std::string(buf, len);
	if (command.rfind("\r\n") == (command.size() - 2))
		command = command.substr(0, command.size() - 2);
	if (command.rfind("\n") == (command.size() - 1))
		command = command.substr(0, command.size() - 1);
	if (command.rfind("\r") == (command.size() - 1))
		command = command.substr(0, command.size() - 1);
	if (command == "PASV"){
		this->processPASV();
	}
	else if (command.find("RETER") == 0){
		this->processRETER(command);
	}
	else if (command.find("STOR") == 0){
		this->processSTOR(command);
	}
	else if (command == "QUIT"){
		this->processQUIT();
	}
	else{
		std::string msg = "no command\n";
		send(ClientSock, msg.c_str(), msg.size(), 0);
	}
}

void Server::processPASV(){
	int result;
	if (commSock == INVALID_SOCKET){
		this->openCommSock();
	}
	std::string str = "227 (" + std::string(TRANSPORT_PORT, 4) + ")\n";
	result = send(ClientSock, str.c_str(), str.size(), 0);
	if (result == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSock);
		WSACleanup();
		return ;
	}
}

void Server::processRETER(std::string command){
	//check file exist or not
	int blank = command.find(" ");
	std::string temp = command.substr(blank+1, command.size() - blank);
	std::string msg;
	this->ifile = std::ifstream(this->root_dir + temp, std::ifstream::binary);
	if (ifile){
		ifile.seekg(0, std::ifstream::end);
		msg = "SIZE " + std::to_string((int)ifile.tellg()) +"\n";
		ifile.seekg(0, std::ifstream::beg);
		this->file_path = this->root_dir + temp;
		this->mode = Mode::download;
	}
	else
		msg = "NO File\n";
	send(ClientSock, msg.c_str(), msg.size(), 0);
}

void Server::processSTOR(std::string command){
	int blank = command.find(" ");
	int blank2 = command.find(" ", blank + 1);
	std::string filename = command.substr(blank + 1, blank2 - blank - 1);
	this->file_path = this->root_dir + filename;
	std::string size = command.substr(blank2 + 1, command.size() - blank2);
	this->fileSize = std::stoi(size);
	std::string msg = "OK\n";
	send(ClientSock, msg.c_str(), msg.size(), 0);
	this->mode = Mode::upload;

}

void Server::processQUIT(){
	closesocket(ClientSock);
	closesocket(commSock);
	WSACleanup();
}

void Server::openCommSock(){
	struct addrinfo hints;
	struct addrinfo *result = NULL;
	//initial winsock
	int iresult = WSAStartup(MAKEWORD(2, 2), &(this->wsaData));
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//Resolve the server address and port
	iresult = getaddrinfo(NULL, TRANSPORT_PORT, &hints, &result);

	// Create a SOCKET for connecting to server
	this->ListenSock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (this->ListenSock == INVALID_SOCKET) {
		std::cout << "socket failed with error : " << WSAGetLastError() << std::endl;
		WSACleanup();
		return ;
	}
	iresult = bind(this->ListenSock, result->ai_addr, (int)result->ai_addrlen);
	freeaddrinfo(result);

	iresult = listen(this->ListenSock, 1); // connect amount: 1
	//create thread
	unsigned int myCounter = 0;
	DWORD id;
	HANDLE myHandle = CreateThread(NULL, 0, this->DataThread, this, 0, &id);
}

DWORD Server::runComm(){
	this->commSock = accept(this->ListenSock, NULL, NULL);
	if (this->commSock == INVALID_SOCKET) {
		std::cout << "socket failed with error : " << WSAGetLastError() << std::endl;
		closesocket(ListenSock);
		WSACleanup();
		return 0;
	}
	while (1){
		if (this->mode == Mode::download){
			this->sendfile();
			this->mode = Mode::normal;
		}
		else if (this->mode == Mode::upload){
			this->recvfile();
			this->mode = Mode::normal;
		
		}
		else{
			//do nothing
		}
	}
	return 0;
}

void Server::sendfile(){
	int result; 
	char temp[DEFAULT_BUFLEN];
	while (!this->ifile.eof()){
		this->ifile.read(temp, DEFAULT_BUFLEN);
		result = send(this->commSock, temp, this->ifile.gcount(), 0);
		if (result == SOCKET_ERROR)
			std::cout << "error\n";
	}
}

void Server::recvfile(){
	int result = 0;
	int recvbuflen = DEFAULT_BUFLEN;
	int total = 0;
	char recvbuf[DEFAULT_BUFLEN];
	this->ofile.open(this->file_path, std::ifstream::binary);
	do{
		result = recv(commSock, recvbuf, recvbuflen, 0);
		if (result>0)
			this->ofile.write(recvbuf, result);
		total += result;
		if (total >= this->fileSize)
			break;
	} while (result > 0);
	this->ofile.close();
}

DWORD WINAPI Server::DataThread(LPVOID p){
	return ((Server*)p)->runComm();
}