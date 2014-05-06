sidescroll: sidescroll.o
	g++ -ggdb -lzmq -lpthread -o $@ $<

%.o: %.c++
	g++ -ggdb -W -Wall -Wextra -pedantic -std=c++11 -c -o $@ $<
