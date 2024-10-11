CPP_FLAGS='-std=c++20'
all: http_conn.o server

http_conn.o: http_conn.cc http_conn.hh locker.hh
    g++ ${CPP_FLAGS} -c http_conn.cc -o http_conn.o

server: server.cc http_conn.o
    g++ ${CPP_FLAGS} -o server server.cc http_conn.o

