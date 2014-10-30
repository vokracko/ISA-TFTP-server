#ifndef H_PARAMS
#define H_PARAMS

#include <vector>
#include <tuple>
#include <string>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <arpa/inet.h>
#include <thread>

class Params
{
	public:
		static unsigned short DEFAULT_PORT;
		static int NOT_SET; // není vytvořen socket

		using fullAddr = std::tuple<std::string, unsigned short, bool, int, std::thread*>; // adresa, port, ipv6, socket
		using fullAddrVector = std::vector<Params::fullAddr>;
		fullAddrVector addresses;
		std::string dir;
		std::string addr;
		int blocksize = 512;
		int timeout = 3;

		void parseAddresses(std::string src);
		fullAddr parseAddress(std::string src, unsigned short defaultPort);
		unsigned int parseInt(const char * ptr);
		bool valid();
		void print();
};

#endif
