CPPFLAGS+=-std=c++11 -Wall -Wextra -Wpedantic -O2

all: iPerfer

serverTest: iPerfer
	./iPerfer -s -p 64567

clientTest: iPerfer
	./iPerfer -c -h 127.0.0.1 -p 64567 -t 5
