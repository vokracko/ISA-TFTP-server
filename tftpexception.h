#ifndef H_TFTPEXCEPTION
#define H_TFTPEXCEPTION 1

#include <string>
#include <stdexcept>
#include <cstdio>
#include <cerrno>
#include <cstring>

class TFTPException: public std::exception
{
	private:
		int code;
		int err;

	public:

		static const int NOT_SET;
		static const int SOCKET;

		static const unsigned short ERR_UNDEFINED = 0;
		static const unsigned short ERR_NOTFOUND = 1;
		static const unsigned short ERR_ACCESS = 2;
		static const unsigned short ERR_FULL = 3;
		static const unsigned short ERR_ILLEGAL = 4;
		static const unsigned short ERR_UNKNOWN = 5;
		static const unsigned short ERR_EXISTS = 6;
		static const unsigned short ERR_USER = 7;
		static const unsigned short ERR_OPTION = 8;

		TFTPException(int code, int err = 0);
		std::string what();
		int getCode();
};


#endif
