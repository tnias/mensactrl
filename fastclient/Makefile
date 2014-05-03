flash: flash.o
	g++ -lzmq -o $@ $<

snow: snow.o
	g++ -lzmq -o $@ $<

client: client.o
	g++ -lzmq -o $@ $<

%.o: %.c
	g++ -W -Wall -Wextra -pedantic -std=c++11 -c -o $@ $<
