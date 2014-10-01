#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>



class Params
{
	public:
		using fullAddr = std::pair<std::string, unsigned short>;
		using fullAddrVector = std::vector<Params::fullAddr>;
		std::string dir;
		std::string addr = "localhost,69";
		std::string multicast;
		int size = 0;
		int timeout = 0;

		Params::fullAddrVector parseAddress(std::string & src);
		unsigned int parseInt(const char * ptr);
		bool paramsSet();
};
