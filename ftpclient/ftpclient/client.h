#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define COMMAND_PORT "1025"
#define COMMUNICATION_PORT "1026"
#define SERVER_FTP_PORT "21"
#define DEFAULT_BUFLEN 512

enum Mode{
	normal = 0,
	download,
	upload
};

class Client
{
public:
	Client();
	~Client();
	bool intital(std::string addr);
	void run();
private:
	void processCommand();
	void processMessage(char* buf, int len);
	bool processPASV(std::string &msg);
	bool processRETER(std::string &msg);
	bool uploadFile();
	bool downloadFile();
	bool connectComm(std::string port);
	DWORD runComm();
	static DWORD WINAPI DataThread(LPVOID p);
private:
	SOCKET cmdSock;
	SOCKET commSock;
	WSADATA wsaData;
	addrinfo *cmdaddr;
	addrinfo *commaddr;
	std::string addr;
	Mode mode;
	std::ofstream ofile;
	std::string file_path;
	int fileSize;
	std::string cmd;
};