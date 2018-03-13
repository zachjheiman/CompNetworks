CPPFLAGS+=-std=c++11 -Wall -Wextra -Wpedantic -O2

all: miProxy

proxy: miProxy
	./miProxy log.txt 0.1 4500 127.0.0.1

server:
	/usr/local/apache2/bin/apachectl start

firefox:
	python launch_firefox.py 1
