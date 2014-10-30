#include "tftpprotocolexception.h"

unsigned short TFTPProtocolException::UNDEFINED = 0;
unsigned short TFTPProtocolException::NOTFOUND = 1;
unsigned short TFTPProtocolException::ACCESS = 2;
unsigned short TFTPProtocolException::FULL = 3;
unsigned short TFTPProtocolException::ILLEGAL = 4;
unsigned short TFTPProtocolException::UNKNOWN = 5;
unsigned short TFTPProtocolException::EXISTS = 6;
unsigned short TFTPProtocolException::USER = 7;
unsigned short TFTPProtocolException::OPTION = 8;

TFTPProtocolException::TFTPProtocolException(int code, int err) : TFTPException(code, err)
{

}
