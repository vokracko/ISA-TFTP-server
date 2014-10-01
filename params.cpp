#include "params.h"

Params::fullAddrVector Params::parseAddress(std::string & src)
{
	Params::fullAddrVector result;
	std::size_t posSemicolon, posComma;
	std::string single, address;
	unsigned short port;

	posSemicolon = src.find(';');

	while(posSemicolon != std::string::npos)
	{
		single = src.substr(0, posSemicolon);
		posComma = single.find(',');

		if(posComma == std::string::npos)
		{
			throw std::invalid_argument("Address");
		}

		address = single.substr(0, posComma);
		port = this->parseInt(single.c_str() + posComma);

		result.push_back(Params::fullAddr(address, port));

		src = src.substr(0, posSemicolon);
		posSemicolon = src.find(';');
	}

	return result;
}

unsigned int Params::parseInt(const char * ptr)
{
	std::size_t pos;
	unsigned int result = std::stoi(std::string(ptr), &pos);

	if(strlen(ptr) != pos || result <= 0)
	{
		throw std::invalid_argument("Number");
	}

	return result;

}

bool Params::paramsSet()
{
	return !this->dir.empty() && !this->addr.empty() && this->timeout && this->size;
}
