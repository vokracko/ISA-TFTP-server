#include "tftpclient.h"

/**
 * @brief Create new socket, start new thread
 * @param address address of interface
 * @param inaddr client sockaddr
 * @param buffer first message from client
 * @param socklen size of inaddr
 */
TFTPClient::TFTPClient(std::string & address, sockaddr * inaddr, socklen_t socklen, char * buffer, int length)
{
	unsigned int requiredLength;
	this->ipv6 = socklen == sizeof(sockaddr_in6);
	this->socklen = socklen;
	this->inaddr = inaddr;
	this->saveClientAddress(inaddr, ipv6);

	try
	{
		this->sck = TFTPServer::createSocket(address, 0, ipv6);
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
 */
void TFTPClient::setDefaults(int timeout, int blocksize)
{
	this->maxBlocksize = blocksize;
	this->maxTimeout = timeout;
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
	char output[length + 2] = {0};

	this->twoByte(blockid, output);
	memcpy(output+2, data, length);
	this->message(DATA, output, length + 2);
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
}

/**
 * @brief Send ack of options
 */
 void TFTPClient::oack()
{
	std::vector<unsigned char> data;

	for(optionVector::iterator it = this->options.begin(); it!= this->options.end(); ++it)
	{
		std::copy( it->first.begin(), it->first.end(), std::back_inserter(data));
		data.push_back('\0');

		if(this->opcode == RRQ && it->first == "tsize")
		{
			it->second = std::to_string(this->tsize);
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
	char message[length+2] = {0};

	this->twoByte(opcode, message);
	memcpy(message + 2, data, length);

	std::string debugMsg = "sending: opcode=";
	debugMsg += this->opcode2str(opcode);

	if(opcode != OACK)
	{
		debugMsg += ", blockid=";
		debugMsg += std::to_string(message[3] | message[2] << 8);
	}

	this->debug(debugMsg);

	sendto(this->sck, message, length + 2, 0, this->inaddr, this->socklen);
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

	this->opcode = data[1] | data[0] << 8;
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

	this->filename.assign(filename);

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
void TFTPClient::debug(std::string & msg)
{
	std::cout << this->addressPort << " # " << msg << std::endl;
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

void TFTPClient::isUnique(int val)
{
	if(val != UNDEFINED)
	{
		throw TFTPProtocolException(TFTPProtocolException::OPTION);
	}
}

void TFTPClient::tryFile()
{
	std::ofstream file(this->filename);

	if(!file) //TODO && eneoug space
	{
		throw TFTPProtocolException(TFTPProtocolException::EXISTS); // TODO or not access righgt
	}


	file.close();
}

/**
 * @brief Start data packet flow
 */
void TFTPClient::proceed()
{
	if(this->timeout == UNDEFINED) this->setTimeout(this->maxTimeout);
	if(this->blocksize == UNDEFINED) this->setBlocksize(this->maxBlocksize);

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
	std::ifstream file(this->filename);
	unsigned int i = 1;
	int result;
	char data[this->blocksize];

	if(!file)
	{
		throw TFTPProtocolException(TFTPProtocolException::NOTFOUND);
	}

	if(!this->options.empty())
	{
		do
		{
			this->oack();
			result = this->recvAck(0);
		} while(result == RETRY);
	}

	while(!file.eof())
	{
		file.read(data, this->blocksize);

		do
		{
			this->data(i, data,  file.gcount());
			result = this->recvAck(i);
		} while(result == RETRY);

		++i;
	}
}

/**
 * @brief Handle write request from client, receive data, send acks
 */
void TFTPClient::wrq()
{
	std::ofstream file(this->filename);
	unsigned int i = 1;
	int bytes;
	char data[this->blocksize + 4] = {0}; // 2B pro tftp hlavičku

	this->tryFile();
	this->wrqReply(0);

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

		file.write(data+4, bytes);
		++i;

		if(!file.good())
		{
			throw TFTPProtocolException(TFTPProtocolException::FULL);
		}

		if(i == (1 << 16) - 1)
		{
			throw TFTPProtocolException(TFTPProtocolException::ACCESS); // překročen maximální počet bloků
		}


	} while(bytes == this->blocksize);
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
	} while(memcmp(this->inaddr, sockptr, this->socklen) != 0);

	if(bytes == ETIMEDOUT)
	{
		return RETRY;
	}
	else if(bytes < 4)
	{
		throw TFTPProtocolException(TFTPProtocolException::ILLEGAL);
	}


	opcoderecv = data[0] << 8 | data[1];
	blockidrecv = data[2] << 8 | data[3];

	if(opcode == ERROR || opcode != opcoderecv || blockid != blockidrecv)
	{
		throw TFTPProtocolException(TFTPProtocolException::ILLEGAL);
	}

	std::string debugMsg = "receiving: opcode=";
	debugMsg += this->opcode2str(opcode);
	debugMsg += ", blockid=";
	debugMsg += std::to_string(blockid);
	this->debug(debugMsg);

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
		// TODO check disk space
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

	if(this->blocksize < 8 || this->blocksize > 65464)
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

	file.close();

	return file.tellg();
}

void TFTPClient::tsizeCheck()
{
	if(this->tsize == UNDEFINED) this->tsize = this->filesize(this->filename);

	if(this->tsize > (1 << 16) * this->blocksize)
	{
		throw TFTPProtocolException(TFTPProtocolException::ACCESS); // file is too big for transmision
	}
}
