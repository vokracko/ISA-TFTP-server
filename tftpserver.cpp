#include "tftpserver.h"

TFTPServer::TFTPServer()
{

}

TFTPServer::~TFTPServer()
{
	this->mainLock.lock();
}

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
		std::tie(address, port, ipv6, std::ignore, std::ignore) = (*it);
		sck = TFTPServer::createSocket(address, port, ipv6);
		std::get<3>(*it) = sck;
	}

	params.print();
	this->params = params;
	this->mtu(sck);
}

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

void TFTPServer::shutdown()
{
	int sck;
	std::thread * thread;

	for(Params::fullAddrVector::iterator it = this->params.addresses.begin(); it != this->params.addresses.end(); ++it)
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

void TFTPServer::start()
{
	for(Params::fullAddrVector::iterator it = this->params.addresses.begin(); it != this->params.addresses.end(); ++it)
	{
		std::thread * thread = new std::thread(&TFTPServer::socketListen, this, *it);
		std::get<4>(*it) = thread;
	}
}

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

	while(true) //TODO doplnit správnou podmínku
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
			break;
		}

		client = new TFTPClient(address, inaddr, socklen, buffer, bytes);
		std::thread thread(&TFTPServer::clientThread, this, client);
		thread.detach();
		memset(buffer, 0, 513);
	}
}

void TFTPServer::clientThread(TFTPClient * client)
{
	this->clientLock.lock();

	if(this->clientCount++ == 0)
	{
		this->mainLock.lock();
	}

	this->clientLock.unlock();

	client->setDefaults(this->params.timeout, this->params.blocksize);
	client->work();

	this->clientLock.lock();

	if(--this->clientCount == 0)
	{
		this->mainLock.unlock();
	}

	this->clientLock.unlock();
	delete client;
}


void TFTPServer::mtu(int sck)
{
	int size  = 0;
	ifreq *ifr;
	ifconf ifc;
	ifc.ifc_len = 0;
	ifc.ifc_req = NULL;

	do
	{
		size += sizeof(ifreq);
		ifc.ifc_req = (ifreq*) realloc(ifc.ifc_req, size);
		//todo kontroloa jestli je povedlo
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

		std::cout << ifr->ifr_name << "=" << ifr->ifr_mtu << "B ";
	}

	std::cout << std::endl;

	delete ifc.ifc_req;
}
