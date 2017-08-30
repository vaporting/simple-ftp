#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "21"
#define TRANSPORT_PORT "5566"
#define DEFAULT_BUFLEN 512

enum Mode{
	normal = 0,
	download,
	upload
};

class Server{
public:
	Server();
	~Server();
	bool initial();
	void run();
private:
	void communication();
	void processCommand(char* buf, int len);
	void processPASV();
	void processRETER(std::string command);
	void processSTOR();
	void processQUIT();
	void openCommSock();
	DWORD runComm();
	void sendfile();
	void recvfile();
	static DWORD WINAPI DataThread(LPVOID p);
private:
	SOCKET ListenSock;
	SOCKET ClientSock;
	SOCKET commSock;
	WSADATA wsaData;
	Mode mode;
	std::string root_dir;
	std::string file_path;
	std::ifstream ifile;
};