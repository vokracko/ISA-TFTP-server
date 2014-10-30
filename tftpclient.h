
class TFTPClient;

#ifndef H_TFTPCLIENT
#define H_TFTPCLIENT

#include "tftpserver.h"
#include "tftpprotocolexception.h"
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <fstream>

class TFTPClient
{

	const int UNDEFINED = -1;
	const int RRQ = 1;
	const int WRQ = 2;
	const int DATA = 3;
	const int ACK = 4;
	const int ERROR = 5;
	const int OACK = 6;
	const int NETASCII = 7;
	const int OCTET = 8;
	const int RETRY = -2;
	const int CONTINUE = -3;

	using option = std::pair<std::string, std::string>;
	using optionVector = std::vector<option>;

	sockaddr * inaddr;
	socklen_t socklen;

	int sck;
	bool ipv6;

	int mode = UNDEFINED;
	int tsize = UNDEFINED; //transfer size
	int timeout = UNDEFINED;
	int blocksize = UNDEFINED;
	int maxBlocksize;
	int maxTimeout;
	unsigned short opcode;
	std::string filename;

	optionVector options;
	bool failed = false;

	std::string addressPort;

	public:
		TFTPClient(std::string & address, sockaddr * inaddr, socklen_t socklen, char * buffer, int length);
		~TFTPClient();
		void work();
		void setDefaults(int timeout, int blocksize);

	private:
		void tsizeCheck();
		std::string opcode2str(unsigned short opcode);
		void debug(std::string & msg);
		void strtolower(char * str);
		void saveClientAddress(sockaddr * inaddr, bool ipv6);
		int recvAck(unsigned int blockid);
		void wrqReply(unsigned int i);
		int recvData(char * data, unsigned int blockid);
		int recv(char * data, unsigned int length, unsigned short opcode, unsigned short blockid);
		bool setTimeout(int seconds);
		int setBlocksize(int blocksize);
		void isUnique(int val);
		void setTsize(int tsize);
		int filesize(std::string & filename);
		void message(unsigned short opcode, const void * data, unsigned int length);
		void oack();
		void error(unsigned short errcode);
		void ack(unsigned short blockid);
		void data(unsigned short blockid, const char * data, unsigned int length);
		void proceed();
		void optional(char * data, unsigned int length);
		unsigned int required(const char * data);
		void rrq();
		void wrq();
		void twoByte(unsigned short num, char * result);
		void tryFile();
};

#endif
