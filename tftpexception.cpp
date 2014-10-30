#include "tftpexception.h"
#include <iostream>

const int TFTPException::NOT_SET = 0;
const int TFTPException::SOCKET = 1;

TFTPException::TFTPException(int code, int err) : std::exception()
{
	this->code = code;
	this->err = err;
}

std::string TFTPException::what()
{
	static std::string messages[] =
	{
		"Missing argument(s)",
		"Socket() problem"
	};

	std::string result = messages[this->code];

	if(this->err)
	{
		result.append("\r\n");
		result.append(strerror(this->err));
	}

	return result;
}

int TFTPException::getCode()
{
	return this->code;
}
