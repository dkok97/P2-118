CC=g++
CPPFLAGS=-g -Wall
USERID=204818138

default: clean server client

server:
	$(CC) -o server $^ $(CPPFLAGS) server.cpp

client:
	$(CC) -o client $^ $(CPPFLAGS) client.cpp

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM server client *.tar.gz

# dist: tarball
#
# tarball: clean
# 	tar -cvzf $(USERID).tar.gz webserver.c Makefile README report.pdf
