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
#include <sys/types.h>
#include <ifaddrs.h>
#include <map>
#include <mutex>

class TFTPServer
{
	static Params params;
	static std::mutex shutdownLock;
	std::mutex mainLock;
	std::mutex clientLock;
	unsigned int clientCount = 0;

	public:
		static const int MAX_BLOCKSIZE;

	private:
		void socketListen(Params::fullAddr addr);
		void clientThread(TFTPClient * client);
		void mtu(int sck);

	public:
		static int createSocket(std::string & address, unsigned short port, bool ipv6);
		static void terminate(int sig);

		TFTPServer();
		~TFTPServer();
		void configure(Params & params);
		void start();
		void shutdown();
};

#endif
