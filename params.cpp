#include "params.h"

unsigned short Params::DEFAULT_PORT = 69;
int Params::NOT_SET = -1;

/**
 * @brief Split src by semicolons
 * @param src source string
 * @return array of pairs address[,port]
 */
void Params::parseAddresses(std::string src)
{
	std::size_t pos = src.find('#');
	bool next = true;
	std::string single;

	do
	{
		if(pos == std::string::npos)
		{
			next = false;
		}

		single = src.substr(0, pos);
		this->addresses.push_back(this->parseAddress(single, Params::DEFAULT_PORT));

		src = src.substr(pos + 1);
		pos = src.find('#');

	} while(next);
}


/**
 * @brief Split string by comma (address[,port])
 * @param src String containing single pair of address[,port]
 * @param defaultPort port is optional, there can be only address in src
 *
 * @return tuple of <address, port, ipv6flag>
 */
Params::fullAddr Params::parseAddress(std::string src, unsigned short defaultPort)
{
	std::string address;
	std::size_t pos;
	sockaddr_in addr_ipv4;
	sockaddr_in6 addr_ipv6;
	int result;
	unsigned short port;
	char _address[INET6_ADDRSTRLEN] = {0};
	addrinfo * addr_info;


	pos = src.find(',');

	if(pos == std::string::npos)
	{
		port = defaultPort;
	}
	else
	{
		port = this->parseInt(src.c_str() + pos + 1);
	}

	address = src.substr(0, pos);
	result = getaddrinfo(address.c_str(), NULL, NULL, &addr_info);

	if(result != 0) throw std::invalid_argument("address");

	if(addr_info->ai_family == AF_INET)
	{
		inet_ntop(AF_INET, &((sockaddr_in *) addr_info->ai_addr)->sin_addr, _address, sizeof(_address));
	}
	else
	{
		inet_ntop(AF_INET6, &((sockaddr_in6 *) addr_info->ai_addr)->sin6_addr, _address, sizeof(_address));
	}

	freeaddrinfo(addr_info);
	address = _address;

	result = inet_pton(AF_INET, address.c_str(), &(addr_ipv4.sin_addr));

	if(result == 1)
	{
		return fullAddr(address, port, false, Params::NOT_SET, nullptr, Params::NOT_SET);
	}

	result = inet_pton(AF_INET6, address.c_str(), &(addr_ipv6.sin6_addr));

	if(result == 1)
	{
		return fullAddr(address, port, true, Params::NOT_SET, NULL, Params::NOT_SET);
	}

	throw std::invalid_argument("address");

}

/**
 * @brief Convert ansi string from ptr to number
 * @param ptr pointer to string
 * @return converted number
 * @throws std::invalid_argument, std::out_of_range
 */
unsigned int Params::parseInt(const char * ptr)
{
	std::size_t pos;
	unsigned int result = std::stoi(std::string(ptr), &pos);

	if(strlen(ptr) != pos || result <= 0)
	{
		throw std::invalid_argument("number format");
	}

	return result;

}

/**
 * @brief Are all required parameters set?
 * @return
 */
bool Params::valid()
{
	if(this->addresses.empty())
	{
		fullAddr addr("127.0.0.1", 69, false, Params::NOT_SET, nullptr, Params::NOT_SET);
		this->addresses.push_back(addr);
	}

	return !this->dir.empty();
}

/**
 * @brief Print server setup
 */
void Params::print()
{

	std::cout << "Directory: " << this->dir << std::endl;
	// First address
	std::cout << "Address: " << std::get<0>(this->addresses.at(0)) << ":" << std::get<1>(this->addresses.at(0));

	// Rest of addresses separated by comma
	for(fullAddrVector::iterator it = this->addresses.begin() + 1; it != this->addresses.end(); ++it)
	{
		std::cout <<  ", " << std::get<0>(*it) << ":" << std::get<1>(*it);
	}

	std::cout << std::endl;

	if(this->timeout)
	{
		std::cout << "Max. timeout: " << this->timeout << "s" << std::endl;
	}
}
