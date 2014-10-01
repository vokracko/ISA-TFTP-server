
//mytftpserver -d cesta -a adresa1,port1;adresa2,port2 -t timeout -s velikost -m madresa,mport

#include "params.h"
#include "tftpexception.h"
#include <unistd.h>
#include <iostream>

int main(int argc, char* argv[])
{
	int opt;
	Params params;

	while((opt = getopt(argc, argv, "")) != -1)
	{
		switch(opt)
		{
			case 'd': // working directory
				params.dir.assign(optarg);
				break;

			case 'a': // listening address
				params.addr.assign(optarg);
				break;

			case 'm': // multicast address
				params.multicast.assign(optarg);
				break;

			case 's': // size
				params.timeout = params.parseInt(optarg);
				break;

			case 't': // timeout
				params.timeout = params.parseInt(optarg);
				break;
		}
	}

	if(!params.paramsSet())
	{
		std::cerr << TFTPException::notSet << std::endl;
		return 1;
	}

	return 0;
}


