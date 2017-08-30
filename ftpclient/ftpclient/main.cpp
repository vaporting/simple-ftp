#include "client.h"
#include <fstream>

int main(){
	/*std::ofstream ofile;
	ofile.open("123.txt", std::ifstream::binary | std::ifstream::ate);
	ofile.close();*/

	Client client;
	//client.intital("192.168.1.4");

	client.intital("127.0.0.1");
	client.run();
	return 0;
}