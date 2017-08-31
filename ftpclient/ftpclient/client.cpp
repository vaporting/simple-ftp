#include "client.h"
#include <string>

Client::Client(){
	this->cmdSock = INVALID_SOCKET;
	this->commSock = INVALID_SOCKET;
	this->mode = Mode::normal;
}

Client::~Client(){

}

bool Client::intital(std::string addr){
	this->addr = addr;
	struct addrinfo hints;
	struct addrinfo *result = NULL;
	//initial winsock
	int iresult = WSAStartup(MAKEWORD(2, 2), &(this->wsaData));
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	//hints.ai_flags = AI_PASSIVE;

	//Resolve the server address and port
	iresult = getaddrinfo(addr.c_str(), SERVER_FTP_PORT, &hints, &result);

	// Create a SOCKET for connecting to server
	cmdSock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (cmdSock == INVALID_SOCKET) {
		std::cout << "socket failed with error : " << WSAGetLastError() << std::endl;
		WSACleanup();
		return false;
	}
	this->cmdaddr = result;
	return true;
}

void Client::run(){
	int result, i;
	result = connect(cmdSock, this->cmdaddr->ai_addr, (int)(this->cmdaddr)->ai_addrlen);
	if (result == SOCKET_ERROR) {
		closesocket(cmdSock);
		cmdSock = INVALID_SOCKET;
		return;
	}
	freeaddrinfo(this->cmdaddr);
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	do{
		std::getline(std::cin, this->cmd);
		if (this->processCommand()){
			this->cmd += "\n";
			result = send(this->cmdSock, this->cmd.c_str(), this->cmd.size(), 0);
			result = recv(cmdSock, recvbuf, recvbuflen, 0);
			if (result > 0){ //receive buf
				for (i = 0; i < result; i++)
					std::cout << recvbuf[i];
				this->processMessage(recvbuf, result);
			}
			else if (result == 0){
				std::cout << "client closing\n";
			}
			else{
				printf("recv failed with error: %d\n", WSAGetLastError());
			}
		}
	} while (result > 0);
}

bool Client::processCommand(){
	int blank;
	if (this->cmd.find("RETER ") == 0){
		blank = this->cmd.find(" ");
		this->file_path = this->cmd.substr(blank + 1, this->cmd.size() - 1);
	}
	if (this->cmd.find("STOR") == 0){
		blank = this->cmd.find(" ");
		this->file_path = this->cmd.substr(blank + 1, this->cmd.size() - 1);
		this->ifile = std::ifstream(this->file_path, std::ifstream::binary);
		if (this->ifile){
			this->ifile.seekg(0, std::ifstream::end);
			this->cmd = this->cmd + " " + std::to_string((int)(this->ifile.tellg()));
			this->ifile.seekg(0, std::ifstream::beg);
		}
		else{
			std::cout << "no this file" << std::endl;
			this->ifile.close();
			return false;
		}
	}
	return true;
}

void Client::processMessage(char* buf, int len){
	std::string message = std::string(buf, len);
	if (message.find("227") == 0){
		this->processPASV(message);
	}
	if (message.find("SIZE") == 0 && this->cmd.find("RETER") == 0)
		this->processRETER(message);
	if (message.find("OK") == 0 && this->cmd.find("STOR") == 0)
		this->processSTOR();
}

bool Client::processPASV(std::string &msg){
	int begin = msg.find("(");
	int end = msg.find(")");
	std::string port = msg.substr(begin + 1, end - begin - 1);
	if (this->connectComm(port)){

		return true;
	}
	else
		return false;
}

bool Client::processRETER(std::string &msg){
	int blank = msg.find(" ");
	std::string fsize = msg.substr(blank + 1, msg.size() - 1);
	this->fileSize = std::stoi(fsize);
	//this->fileSize = 4; //test
	this->mode = Mode::download;
	return true;
}

bool Client::processSTOR(){
	this->mode = Mode::upload;
	return true;
}


bool Client::downloadFile(){
	int result=0;
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
	return true;
}

bool Client::uploadFile(){
	int result;
	char temp[DEFAULT_BUFLEN];
	while (!this->ifile.eof()){
		this->ifile.read(temp, DEFAULT_BUFLEN);
		result = send(this->commSock, temp, this->ifile.gcount(), 0);
		if (result == SOCKET_ERROR)
			std::cout << "error\n";
	}
	this->ifile.close();
	return true;
}

bool Client::connectComm(std::string port){
	struct addrinfo hints;
	struct addrinfo *result = NULL;
	//initial winsock
	int iresult = WSAStartup(MAKEWORD(2, 2), &(this->wsaData));
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	//hints.ai_flags = AI_PASSIVE;

	//Resolve the server address and port
	iresult = getaddrinfo(this->addr.c_str(), port.c_str(), &hints, &result);

	// Create a SOCKET for connecting to server
	this->commSock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (this->commSock == INVALID_SOCKET) {
		std::cout << "socket failed with error : " << WSAGetLastError() << std::endl;
		WSACleanup();
		return false;
	}
	this->commaddr = result;
	// create thread
	unsigned int myCounter = 0;
	DWORD id;
	HANDLE myHandle = CreateThread(NULL, 0, this->DataThread, this, 0, &id);
	return true;
}

DWORD Client::runComm(){
	int result;
	result = connect(this->commSock, this->commaddr->ai_addr, (int)(this->commaddr)->ai_addrlen);
	if (result == SOCKET_ERROR) {
		std::cout << "socket failed with error : " << WSAGetLastError() << std::endl;
		closesocket(this->commSock);
		this->commSock = INVALID_SOCKET;
		return 0 ;
	}
	freeaddrinfo(this->commaddr);
	while (1){
		if (this->mode == Mode::download){
			this->downloadFile();
			this->mode = Mode::normal;
		}
		else if (this->mode == Mode::upload){
			this->uploadFile();
			this->mode = Mode::normal;
		}
		else{
			//do nothing
		}
	}
	return 0;

}

DWORD WINAPI Client::DataThread(LPVOID p){
	return ((Client*)p)->runComm();
}