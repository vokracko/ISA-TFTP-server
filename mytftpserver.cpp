#include <unistd.h>
#include <iostream>
#include "params.h"
#include "tftpexception.h"
#include "tftpserver.h"
#include <signal.h>


//TODO otestovat
//TODO makefile
//TODO nefunguje timeout

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
	server.shutdown();

	return 0;

}


