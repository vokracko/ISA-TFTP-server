GPP=g++-4.8
FLAGS=-std=c++11 -Wall -Wextra -g3


build: mytftpserver.o tftpserver.o params.o tftpexception.o tftpclient.o tftpprotocolexception.o
	$(GPP) $(FLAGS) -o mytftpserver mytftpserver.o tftpserver.o params.o tftpexception.o tftpclient.o tftpprotocolexception.o -pthread

pack:
	tar xvokra00.tar *.cpp *.h manual.pdf README Makefile

clean:
	rm -rf *.o mytftpserver 2 > /dev/null

%.o: %.cpp
	$(GPP) $(FLAGS) -c $< -o $@
