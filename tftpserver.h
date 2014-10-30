#ifndef H_TFTPSERVER
#define H_TFTPSERVER

#include "params.h"
#include "tftpclient.h"
#include "tftpexception.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <iostream>
#include <cerrno>
#include <vector>
#include <thread>
#include <mutex>

class TFTPServer
{
	Params params;
	std::mutex mainLock;
	std::mutex clientLock;
	unsigned int clientCount = 0;

	private:
		void socketListen(Params::fullAddr addr);
		void clientThread(TFTPClient * client);
		void mtu(int sck);

	public:
		static int createSocket(std::string & address, unsigned short port, bool ipv6);

		TFTPServer();
		~TFTPServer();
		void configure(Params & params);
		void shutdown();
		void start();
};

#endif

//TODO vymyslet jak ukončovat
