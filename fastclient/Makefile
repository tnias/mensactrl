flash: flash.o
	g++ -lzmq -o $@ $<

snow: snow.o
	g++ -lzmq -o $@ $<

client: client.o
	g++ -lzmq -o $@ $<

spectrum: spectrum.o
	g++ -lzmq -lpthread -lasound -lfftw3 -o $@ $<

%.o: %.c
	g++ -W -Wall -Wextra -pedantic -std=c++11 -c -o $@ $<
