#ifndef H_TFTPPROTOCOLEXCEPTION
#define H_TFTPPROTOCOLEXCEPTION 1

#include "tftpexception.h"

class TFTPProtocolException: public TFTPException
{
	public:
		static unsigned short UNDEFINED;
		static unsigned short NOTFOUND;
		static unsigned short ACCESS;
		static unsigned short FULL;
		static unsigned short ILLEGAL;
		static unsigned short UNKNOWN;
		static unsigned short EXISTS;
		static unsigned short USER;
		static unsigned short OPTION;

		TFTPProtocolException(int code, int err = 0);
		std::string what() { return std::string("Error in TFTP protocol comunication");}
};


#endif
