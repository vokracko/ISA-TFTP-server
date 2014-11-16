#include "tftpclient.h"

/**
 * @brief Create new socket, start new thread
 * @param address address of interface
 * @param inaddr client sockaddr
 * @param buffer first message from client
 * @param socklen size of inaddr
 * @param params
 * @param blocksize max blocksize on dev
 */
TFTPClient::TFTPClient(std::string & address, sockaddr * inaddr, socklen_t socklen, char * buffer, int length, Params params, unsigned int blocksize)
{
	unsigned int requiredLength;
	this->ipv6 = socklen == sizeof(sockaddr_in6);
	this->socklen = socklen;
	this->inaddr = inaddr;
	this->saveClientAddress(inaddr, ipv6);

	try
	{
		this->sck = TFTPServer::createSocket(address, 0, ipv6);
		this->setDefaults(params.timeout, params.blocksize == Params::NOT_SET ? blocksize : params.blocksize, params.dir);
		requiredLength = this->required(buffer);
		this->optional(buffer + requiredLength, length - requiredLength);
	}
	catch(TFTPProtocolException & e)
	{
		this->failed = true;
		this->error(e.getCode());
	}
	catch(TFTPException & e)
	{
		this->failed = true;
	}

}

/**
 * @brief Destruct object, release sockaddr structure
 */
TFTPClient::~TFTPClient()
{
	delete this->inaddr;
}

void TFTPClient::saveClientAddress(sockaddr * inaddr, bool ipv6)
{
	unsigned short port;

	if(ipv6)
	{
		char ipAddress[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &(((sockaddr_in6 *) inaddr)->sin6_addr), ipAddress, INET6_ADDRSTRLEN);
		port = ntohs(((sockaddr_in6 *) inaddr)->sin6_port);
		this->addressPort.assign(ipAddress).append(":").append(std::to_string(port));
	}
	else
	{
		char ipAddress[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(((sockaddr_in *) inaddr)->sin_addr), ipAddress, INET_ADDRSTRLEN);
		port = ntohs(((sockaddr_in *) inaddr)->sin_port);
		this->addressPort.assign(ipAddress).append(":").append(std::to_string(port));
	}
}

/**
 * @brief Set default values from cmd arguments
 * @param timeout max acceptable value
 * @param blocksize max acceptable value
 * @param dir working directory
 */
void TFTPClient::setDefaults(int timeout, int blocksize, std::string dir)
{
	this->maxBlocksize = blocksize;
	this->maxTimeout = timeout;
	this->dir = dir;
}

/**
 * @brief Main function of client
 */
void TFTPClient::work()
{
	if(this->failed) return;

	try
	{
		this->proceed();
	} catch(TFTPProtocolException & e)
	{
		this->error(e.getCode());
	} catch(TFTPException & e)
	{

	}

}

/**
 * @brief Send data packet
 * @param blockid
 * @param data
 */
void TFTPClient::data(unsigned short blockid, const char * data, unsigned int length)
{
	char * output = new char[length + 2];
	memset(output, 0, length + 2);

	this->twoByte(blockid, output);
	memcpy(output+2, data, length);
	this->message(DATA, output, length + 2);
	delete[] output;
}

/**
 * @brief Send ack
 * @param blockid
 */
void TFTPClient::ack(unsigned short blockid)
{
	char msg[2];
	this->twoByte(blockid, msg);
	this->message(ACK, msg, 2);
}

/**
 * @brief Send error with errorcode
 * @param errcode
 */
void TFTPClient::error(unsigned short errcode)
{
	char msg[2];
	this->twoByte(errcode, msg);
	this->message(ERROR, msg, 2);
	this->debug(std::string("ERROR: ") + std::to_string(errcode));
}

/**
 * @brief Send ack of options
 * @param file pointer to netascii file, value of tsize must be changed
 */
 void TFTPClient::oack(FILE * file)
{
	std::vector<unsigned char> data;
	int tsize = this->tsize;

	for(optionVector::iterator it = this->options.begin(); it!= this->options.end(); ++it)
	{
		std::copy(it->first.begin(), it->first.end(), std::back_inserter(data));
		data.push_back('\0');

		if(this->opcode == RRQ && it->first == "tsize")
		{
			if(file != NULL)
			{
				fseek(file, 0L, SEEK_END);
				tsize = ftell(file);
				rewind(file);
			}
			it->second = std::to_string(tsize);
		}

		std::copy( it->second.begin(), it->second.end(), std::back_inserter(data));
		data.push_back('\0');
	}

	this->message(OACK, data.data(), data.size());
}

/**
 * @brief Send general message, prepends opcode
 * @param opcode code of operation
 * @param data
 */
void TFTPClient::message(unsigned short opcode, const void * data, unsigned int length)
{
	char * message = new char[length+2];
	memset(message, 0, length + 2);

	this->twoByte(opcode, message);
	memcpy(message + 2, data, length);

	sendto(this->sck, message, length + 2, 0, this->inaddr, this->socklen);
	delete[] message;
}

/**
 * @brief Make two byte string from number
 * @param num
 * @param result
 */
void TFTPClient::twoByte(unsigned short num, char * result)
{
	result[1] = num;
	result[0] = num >> 8;
}

/**
 * @brief Only in initial packet
 * @details [long description]
 *
 * @param data [description]
 * @return [description]
 */
unsigned int TFTPClient::required(const char * data)
{
	char filename[512] = {0};
	char mode[9] = {0}; //netascii, octet

	this->opcode = ((unsigned char) data[1]) | (unsigned char) data[0] << 8;
	sscanf(data+2, "%s", filename);
	sscanf(data+2+strlen(filename)+1, "%s", mode);

	if(this->opcode != WRQ && this->opcode != RRQ)
	{
		throw TFTPProtocolException(TFTPProtocolException::ILLEGAL);
	}

	strtolower(mode);
	if(strcmp(mode, "netascii") == 0)
	{
		this->mode = NETASCII;
	}
	else if(strcmp(mode, "octet") == 0)
	{
		this->mode = OCTET;
	}
	else
	{
		throw TFTPProtocolException(TFTPProtocolException::ILLEGAL);
	}

	this->filename.assign(this->dir).append("/").append(filename);

	std::string debugMsg = this->opcode == RRQ ? "RRQ" : "WRQ";
	debugMsg += " file=";
	debugMsg += filename;
	debugMsg += ", mode=";
	debugMsg += mode;
	this->debug(debugMsg);

	return 2 + strlen(filename) + 1 + strlen(mode) + 1; //délka povinných parametrů
}

/**
 * @brief Print debug info
 * @param msg message
 */
void TFTPClient::debug(std::string msg)
{
	std::time_t now = std::time(NULL);
	tm * info;
	char buffer[80] = {0};

	info = localtime(&now);
	strftime(buffer, 80, "%Y-%m-%d %H:%m:%S", info);
	std::cout << "[" << buffer << "] " << this->addressPort << " # " << msg << std::endl;
}

/**
 * @brief Make lower case
 * @param str original string
 */
void TFTPClient::strtolower(char * str)
{
	while(*str != '\0') *str = tolower(*str), str++;
}

/**
 * @brief Recursive handle optional parameters from initial packet
 * @param data
 * @param length of data
 */
void TFTPClient::optional(char * data, unsigned int length)
{
	std::string key, value;
	unsigned int substracted;
	int numvalue;
	bool save = true;

	if(!length) // no more parameters
	{
		return;
	}

	key.assign(data); // handle key
	substracted = key.length() + 1;

	if(substracted == length)
	{
		throw TFTPProtocolException(TFTPProtocolException::OPTION);
	}

	value.assign(data + key.length() + 1); // handle value
	substracted += value.length() + 1;
	numvalue = std::stoi(value);

	if(key == "tsize") this->setTsize(numvalue);
	else if(key == "timeout") save = this->setTimeout(numvalue);
	else if(key == "blksize") numvalue = this->setBlocksize(numvalue);
	else save = false; // unknown

	if(save)
	{
		std::string debugMsg = "optional: ";
		debugMsg += key;
		debugMsg += "=";
		debugMsg += value;
		this->debug(debugMsg);

		this->options.push_back(option(key, std::to_string(numvalue)));
	}

	this->optional(data + substracted, length - substracted);
}

/**
 * @brief Value was not defined earlier
 * @param val value
 */
void TFTPClient::isUnique(int val)
{
	if(val != UNDEFINED)
	{
		throw TFTPProtocolException(TFTPProtocolException::OPTION);
	}
}

/**
 * @brief Check if file can be saved
 */
void TFTPClient::tryFile()
{
	std::ifstream ifile(this->filename);
	std::ofstream ofile(this->filename);

	if(ifile)
	{
		throw TFTPProtocolException(TFTPProtocolException::EXISTS);
	}

	if(!ofile.good())
	{
		throw TFTPProtocolException(TFTPProtocolException::ACCESS);
	}

	this->enoughSpace();
}

/**
 * @brief Check if there is enough disk space for saving file
 */
void TFTPClient::enoughSpace()
{
	if(this->tsize != 0) return;

	struct statvfs buf;
	statvfs(this->dir.c_str(), &buf);

	if(buf.f_bsize * buf.f_bfree <  (unsigned int) this->tsize)
	{
		throw TFTPProtocolException(TFTPProtocolException::FULL);
	}
}


/**
 * @brief Start data packet flow
 */
void TFTPClient::proceed()
{
	if(this->timeout == UNDEFINED) this->setTimeout(3);
	if(this->blocksize == UNDEFINED) this->setBlocksize(512);

	this->tsizeCheck();

	if(this->opcode == RRQ)
	{
		this->rrq();
	}
	else
	{
		this->wrq();
	}

	std::string debugMsg = "Transfer complete";
	this->debug(debugMsg);
}

/**
 * @brief Handle read request from client (send file, receive acks)
 */
void TFTPClient::rrq()
{
	std::FILE * file;
	unsigned int i = 1;
	int result;
	char data[this->blocksize];

	this->debug("Sending data");

	if(this->mode == NETASCII)
	{
		file = this->toNetascii(this->filename);
	}
	else
	{
		file = fopen(this->filename.c_str(), "r");
	}

	if(file == NULL)
	{
		throw TFTPProtocolException(TFTPProtocolException::NOTFOUND);
	}

	if(!this->options.empty())
	{
		do
		{
			this->oack(file);
			result = this->recvAck(0);
		} while(result == RETRY);
	}

	while(!feof(file))
	{
		result = fread(data, 1, this->blocksize, file);

		do
		{
			this->data(i, data, result);
			result = this->recvAck(i);
		} while(result == RETRY);

		++i;
	}

	fclose(file);
}

/**
 * @brief Handle write request from client, receive data, send acks
 */
void TFTPClient::wrq()
{
	std::FILE * file;
	unsigned int i = 1;
	int bytes;
	int result;
	char * data = new char[this->blocksize + 5];
	memset(data, 0, this->blocksize + 5);

	this->tryFile();

	if(this->mode == NETASCII)
	{
		file = std::tmpfile();
	}
	else
	{
		file = fopen(this->filename.c_str(), "wb");
	}

	this->wrqReply(0);

	this->debug("Receiving data");

	do
	{
		do
		{
			bytes = this->recvData(data, i);
			if(bytes == RETRY)
			{
				this->wrqReply(i-1);
				continue;
			}

			this->ack(i);

		} while(bytes == RETRY);

		result = fwrite(data+4, 1, bytes, file);
		++i;

		if(result != bytes)
		{
			fclose(file);
			throw TFTPProtocolException(TFTPProtocolException::FULL);
		}

		if(i == (1 << 16) - 1)
		{
			fclose(file);
			throw TFTPProtocolException(TFTPProtocolException::ACCESS); // překročen maximální počet bloků
		}

	} while(bytes == this->blocksize);

	if(this->mode == NETASCII)
	{
		this->fromNetascii(file);
	}

	fclose(file);

	delete[] data;
}

/**
 * @brief Netascii to machine format
 * @param in temporary file in netascii encoding
 */
void TFTPClient::fromNetascii(std::FILE * in)
{
	std::ofstream out(this->filename);
	std::string line;
	std::size_t pos = 0;
	int c;

	while((c = fgetc(in)) != EOF)
	{
		if(c == '\n' && line.length() && line.back() == '\r')
		{
			line.pop_back();

			while((pos = line.find("\r\0", pos)) != std::string::npos)
			{
				line.replace(pos, 2, "\r");
				pos++;
			}

			out << line << std::endl;
			line.clear();
		}
		else
		{
			line.push_back(c);
		}
	}
}

/**
 * @brief Convert machine format to netascii tmp file
 * @param name filename
 * @return file descriptor to tmp file
 */
std::FILE * TFTPClient::toNetascii(std::string name)
{
	std::FILE * out = std::tmpfile();
	std::ifstream in(name);
	std::string line;
	std::size_t pos = 0;

	while(getline(in, line, '\n'))
	{
		while((pos = line.find('\r', pos)) != std::string::npos)
		{
			line.replace(pos, 1, "\r\0");
			pos++;
		}
	}

	return out;
}

void TFTPClient::wrqReply(unsigned int i)
{
	if(i == 0)
		{
		if(!this->options.empty())
		{
			this->oack();
		}
		else
		{
			this->ack(0);
		}
	}
	else
	{
		this->ack(i);
	}
}

/**
 * @brief Receive data block from client
 * @param blockid
 * @return bytes of data received
 */
int TFTPClient::recvData(char * data, unsigned int blockid)
{
	int bytes;

	bytes = this->recv(data, this->blocksize + 4, DATA, blockid); // 2B tftp hlavička

	return bytes - 4;
}

/**
 * @brief Receive ACK from client on data block
 * @param blockid
 * @return bytes read;
 */
int TFTPClient::recvAck(unsigned int blockid)
{
	char data[5] = {0};
	int bytes;

	bytes = this->recv(data, 4, ACK, blockid);

	return bytes;
}

/**
 * @brief Receive data from socket and validate opcode and blockid, ERROR opcode is permited
 * @param data pointer to allocated memory
 * @param length size of memory
 * @param opcode
 * @param blockid
 * @return bytes received
 */
int TFTPClient::recv(char * data, unsigned int length, unsigned short opcode, unsigned short blockid)
{
	int bytes;
	unsigned short opcoderecv;
	unsigned short blockidrecv;
	sockaddr_in6 inaddr6;
	sockaddr_in inaddr;
	sockaddr * sockptr = ipv6 ? (sockaddr *) &inaddr6 : (sockaddr *) &inaddr;

	do
	{	// skip everything which was not send by original client
		bytes = recvfrom(this->sck, data, length, 0, sockptr, &this->socklen);
	} while(bytes >= 4 && memcmp(this->inaddr, sockptr, this->socklen) != 0);

	if(bytes == ETIMEDOUT)
	{
		return RETRY;
	}
	else if(bytes < 4)
	{
		throw TFTPProtocolException(TFTPProtocolException::ILLEGAL);
	}

	opcoderecv = ((unsigned char) data[0]) << 8 | (unsigned char) data[1];
	blockidrecv = ((unsigned char) data[2]) << 8 | (unsigned char) data[3];

	if(opcode == ERROR || opcode != opcoderecv || blockid != blockidrecv)
	{
		throw TFTPProtocolException(TFTPProtocolException::ILLEGAL);
	}

	return bytes;
}

/**
 * @brief Make string from opcode constant
 * @param opcode
 * @return string value of opcode
 */
std::string TFTPClient::opcode2str(unsigned short opcode)
{
	switch(opcode)
	{
		case 1: return std::string("RRQ");
		case 2: return std::string("WRQ");
		case 3: return std::string("DATA");
		case 4: return std::string("ACK");
		case 5: return std::string("ERROR");
		case 6: return std::string("OACK");
		default: return std::string("INVALID");
	}
}

/**
 * @brief Set timeout on client socket
 * @param seconds timeout value
 * @return save value?
 */
bool TFTPClient::setTimeout(int seconds)
{
	int result;
	timeval timeout;
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;

	this->isUnique(this->timeout);
	this->timeout = seconds;


	if(seconds < 1 || seconds > 255 || seconds > this->maxTimeout)
	{
		return false;
	}

	result = setsockopt(this->sck, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeval));

	if(result < 0)
	{
		throw TFTPException(TFTPException::SOCKET);
	}

	return true;
}

/**
 * @brief Set transfer sie
 * @param tsize value from tftp packet
 */
void TFTPClient::setTsize(int tsize)
{
	this->isUnique(this->tsize);

	if(this->opcode == WRQ) // size of file to be writted
	{
		this->tsize = tsize;
	}
	else // RRQ, size of file which will be send to client
	{
		this->tsize = this->filesize(this->filename);
	}
}

/**
 * @brief Set block size
 * @param blocksize value from tftp packet
 */
int TFTPClient::setBlocksize(int blocksize)
{
	this->isUnique(this->blocksize);

	this->blocksize = blocksize;

	if(this->blocksize < 8 || this->blocksize > TFTPServer::MAX_BLOCKSIZE)
	{
		throw TFTPProtocolException(TFTPProtocolException::OPTION);
	}

	if(blocksize > this->maxBlocksize)
	{
		this->blocksize = maxBlocksize;
	}

	return this->blocksize;
}

/**
 * @brief Get size of the file
 * @param filename name of file
 * @return size
 */
int TFTPClient::filesize(std::string & filename)
{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	if(!file)
	{
		throw TFTPProtocolException(TFTPProtocolException::NOTFOUND);
	}

	return file.tellg();
}

void TFTPClient::tsizeCheck()
{
	if(this->opcode == WRQ && this->tsize == UNDEFINED) return; // WRQ and no tsize option
	if(this->tsize == UNDEFINED) this->tsize = this->filesize(this->filename);

	if(this->tsize > (1 << 16) * this->blocksize)
	{
		throw TFTPProtocolException(TFTPProtocolException::ACCESS); // file is too big for transmision
	}
}
