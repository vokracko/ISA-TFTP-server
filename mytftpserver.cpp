#include <unistd.h>
#include <iostream>
#include "params.h"
#include "tftpexception.h"
#include "tftpserver.h"
#include <signal.h>


void printHelp()
{
	std::cout << "mytftpserver -d cesta [-a adresa1,port1;adresa2,port2 -t timeout -s velikost]" << std::endl;
    std::cout << "\t-d pracovní adresář" << std::endl;
    std::cout << "\t-s blocksize" << std::endl;
    std::cout << "\t-t timeout" << std::endl;
    std::cout << "\t-a adresa" << std::endl;
}

int main(int argc, char* argv[])
{
	std::cout.sync_with_stdio();

	int opt;
	Params params;
	TFTPServer server;

	signal(SIGINT, &TFTPServer::terminate);

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
				default:
					printHelp();
					return 0;
			}
		}
		server.configure(params);

	} catch(std::invalid_argument & e)
	{
		std::cerr << "Invalid argument: " << e.what() << std::endl;
		printHelp();
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
		else
		{
			printHelp();
		}

		std::cerr << e.what() << std::endl;
		return 3;
	}

	server.start();
	server.shutdown();

	return 0;

}


