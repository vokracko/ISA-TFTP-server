#include <unistd.h>
#include <iostream>
#include "params.h"
#include "tftpexception.h"
#include "tftpserver.h"


//TODO ipv6 nefunguje správně bind
//TODO funguje zatím jen přenášení mini souboru směrem ze serveru
//TODO netascii mode překládá do vlastního kódu
//TODO maxblocksize ověření vůči argv a pak podle init paketu, výpis mtu

int main(int argc, char* argv[])
{
	int opt;
	Params params;
	TFTPServer server;;

	try
	{
		while((opt = getopt(argc, argv, "d:a:t:s:")) != -1)
		{
			switch(opt)
			{
				case 'd': // working directory
					params.dir.assign(optarg);
					break;

				case 'a': // listening address
					params.parseAddresses(std::string(optarg));
					break;

				case 's': // size
					params.blocksize = params.parseInt(optarg);
					break;

				case 't': // timeout
					params.timeout = params.parseInt(optarg);
					break;
			}
		}
		server.configure(params);

	} catch(std::invalid_argument & e)
	{
		std::cerr << "Invalid argument: " << e.what() << std::endl;
		return 1;

	} catch(std::out_of_range & e)
	{
		std::cerr << e.what() << std::endl;
		return 2;

	} catch(TFTPException & e)
	{
		if(e.getCode() == TFTPException::SOCKET)
		{
			server.shutdown();
		}

		std::cerr << e.what() << std::endl;
		return 3;
	}

	server.start();

	sleep(10000);

	return 0;

}


