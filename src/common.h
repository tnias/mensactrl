#define CMD_PIXEL 0

struct pixel {
	int x, y;
	uint8_t bright;
};

struct packet {
	uint8_t cmd;
	union {
		struct pixel pixel;
	};
};
