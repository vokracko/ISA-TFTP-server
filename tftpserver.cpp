#include "tftpserver.h"


Params TFTPServer::params;
std::mutex TFTPServer::shutdownLock;
const int TFTPServer::MAX_BLOCKSIZE = 65464;

TFTPServer::TFTPServer()
{

}

TFTPServer::~TFTPServer()
{
	this->mainLock.lock();
}

void TFTPServer::terminate(int sig)
{
	TFTPServer::shutdownLock.unlock();
}

/**
 * @brief Construct sockets and validate parameters
 * @param params parameters
 */
void TFTPServer::configure(Params & params)
{
	std::string address;
	unsigned short port;
	bool ipv6;
	int sck;

	if(!params.valid())
	{
		throw TFTPException(TFTPException::NOT_SET);
	}

	for(Params::fullAddrVector::iterator it = params.addresses.begin(); it != params.addresses.end(); ++it)
	{
		std::tie(address, port, ipv6, std::ignore, std::ignore, std::ignore) = (*it);
		sck = TFTPServer::createSocket(address, port, ipv6);
		std::get<3>(*it) = sck;
	}

	params.print();
	this->params = params;

	if(this->params.blocksize)
	{
		this->mtu(sck);
	}
}

/**
 * @brief Create socket
 * @param address address to lister
 * @param port port number
 * @param ipv6 ip version
 * @return socket descriptor
 */
int TFTPServer::createSocket(std::string & address, unsigned short port, bool ipv6)
{
	sockaddr * addr;
	socklen_t socklen;
	int sck;
	int result;

	if(ipv6)
	{
		sck = socket(AF_INET6, SOCK_DGRAM, 0);

		if(sck < 0)
		{
			throw TFTPException(TFTPException::SOCKET);
		}

		sockaddr_in6 * inaddr = new sockaddr_in6;

		inaddr->sin6_family = AF_INET6;
		inaddr->sin6_port = htons(port);
		socklen = sizeof(sockaddr_in6);
		inet_pton(AF_INET6, address.c_str(), &(inaddr->sin6_addr.s6_addr));

		addr = (sockaddr *) inaddr;
	}
	else
	{
		sck = socket(AF_INET, SOCK_DGRAM, 0);

		if(sck < 0)
		{
			throw TFTPException(TFTPException::SOCKET);
		}

		sockaddr_in * inaddr = new sockaddr_in;

		inaddr->sin_family = AF_INET;
		inaddr->sin_port = htons(port);
		socklen = sizeof(sockaddr_in);
		inet_pton(AF_INET, address.c_str(), &(inaddr->sin_addr.s_addr));

		addr = (sockaddr *) inaddr;
	}

	result = bind(sck, addr, socklen);
	delete addr;

	if(result != 0)
	{
		throw TFTPException(TFTPException::SOCKET, errno);
	}

	return sck;

}

/**
 * @brief Close all listening sockets and their threads
 */
void TFTPServer::shutdown()
{
	this->shutdownLock.lock();

	int sck;
	std::thread * thread;

	for(Params::fullAddrVector::iterator it = TFTPServer::params.addresses.begin(); it != TFTPServer::params.addresses.end(); ++it)
	{
		sck = std::get<3>(*it);

		if(sck == Params::NOT_SET)
		{
			break;
		}

		::shutdown(sck, SHUT_RDWR);
		close(sck);
		thread = std::get<4>(*it);
		thread->join();
		delete thread;
	}
}

/**
 * @brief Run threads for every listening socket
 */
void TFTPServer::start()
{
	for(Params::fullAddrVector::iterator it = this->params.addresses.begin(); it != this->params.addresses.end(); ++it)
	{
		std::thread * thread = new std::thread(&TFTPServer::socketListen, this, *it);
		std::get<4>(*it) = thread;
	}

	this->shutdownLock.lock();
}

/**
 * @brief Listen on socket
 * @param addr parameters
 */
void TFTPServer::socketListen(Params::fullAddr addr)
{
	int sck = std::get<3>(addr);
	int bytes;
	bool ipv6 = std::get<2>(addr);
	char buffer[513] = {0};
	std::string address = std::get<0>(addr);

	TFTPClient * client;
	sockaddr * inaddr;
	socklen_t socklen;

	while(true)
	{
		if(ipv6)
		{
			inaddr = (sockaddr *) new sockaddr_in6;
			socklen = sizeof(sockaddr_in6);
		}
		else
		{
			inaddr = (sockaddr *) new sockaddr_in;
			socklen = sizeof(sockaddr_in);
		}

		bytes = recvfrom(sck, buffer, 513, 0, (sockaddr *) inaddr, &socklen);

		if(bytes == 0)
		{
			delete inaddr;
			break; // socket closed
		}

		client = new TFTPClient(address, inaddr, socklen, buffer, bytes, this->params, std::get<5>(addr));
		std::thread thread(&TFTPServer::clientThread, this, client);
		thread.detach();
		memset(buffer, 0, 513);
	}
}

/**
 * @brief Handle client thread
 * @param client Client object
 */
void TFTPServer::clientThread(TFTPClient * client)
{
	this->clientLock.lock();

	if(this->clientCount++ == 0)
	{
		this->mainLock.lock();
	}

	this->clientLock.unlock();

	client->work();

	this->clientLock.lock();

	if(--this->clientCount == 0)
	{
		this->mainLock.unlock();
	}

	this->clientLock.unlock();
	delete client;
}

/**
 * @brief Print max mtu size on every interface
 * @param sck socket descriptor
 */
void TFTPServer::mtu(int sck)
{
	int size  = 0;
	int result;
	int val;
	ifreq *ifr;
	ifconf ifc;
	ifc.ifc_len = 0;
	ifc.ifc_req = NULL;
	ifaddrs * ifaddrsPtr, * ifaddrsConstPtr;
	std::map<std::string, unsigned int> ifaceMtu;

	std::cout << "Max. blocksize:";

	do
	{
		delete[] ifc.ifc_req;
		size += sizeof(ifreq);
		ifc.ifc_req = (ifreq*) new char[size];
		ifc.ifc_len = size;

		if (ioctl(sck, SIOCGIFCONF, &ifc))
		{
			throw TFTPException(TFTPException::SOCKET);
		}

	} while(size <= ifc.ifc_len);


	ifr = ifc.ifc_req;
	//výpis výsledku
	for (;(char *) ifr < (char *) ifc.ifc_req + ifc.ifc_len; ++ifr)
	{
		if (ioctl(sck, SIOCGIFMTU, ifr) != 0)
		{
			throw TFTPException(TFTPException::SOCKET);
		}

		std::cout << " " << ifr->ifr_name << "=" << ifr->ifr_mtu << "B";
		ifaceMtu.insert(std::make_pair(std::string(ifr->ifr_name), ifr->ifr_mtu));

		if((char*) ifr < (char *) ifc.ifc_req + ifc.ifc_len - sizeof(ifreq))
		{
			std::cout << ",";
		}
	}

	std::cout << std::endl;
	delete[] ifc.ifc_req;

	result = getifaddrs(&ifaddrsPtr);
	ifaddrsConstPtr = ifaddrsPtr;

	if(result != 0)
	{
		throw TFTPException(TFTPException::SOCKET);
	}

	while(ifaddrsPtr != NULL)
	{
		sockaddr_in * inaddr;
		sockaddr_in6 * inaddr6;
		sockaddr * sckaddr;
		std::string address;
		char addrPtr[INET6_ADDRSTRLEN+1] = {0};

		sckaddr = (sockaddr *) ifaddrsPtr->ifa_addr;

		if(sckaddr->sa_family == AF_INET || sckaddr->sa_family == AF_INET6)
		{
			if(sckaddr->sa_family == AF_INET)
			{
				inaddr = (sockaddr_in *) sckaddr;
				inet_ntop(AF_INET, &(inaddr->sin_addr), addrPtr, INET_ADDRSTRLEN);
			}
			else
			{
				inaddr6 = (sockaddr_in6 *) sckaddr;
				inet_ntop(AF_INET6, &(inaddr6->sin6_addr), addrPtr, INET6_ADDRSTRLEN);
			}

			address.assign(addrPtr);

			for(Params::fullAddrVector::iterator it = this->params.addresses.begin(); it != this->params.addresses.end(); ++it)
			{
				if(std::get<0>(*it) == address)
				{
					val = ifaceMtu.find(std::string(ifaddrsPtr->ifa_name))->second;
					std::get<5>(*it) = val > MAX_BLOCKSIZE ? MAX_BLOCKSIZE : val;
				}
			}

		}

		ifaddrsPtr = ifaddrsPtr->ifa_next;
	}

	freeifaddrs(ifaddrsConstPtr);
}
