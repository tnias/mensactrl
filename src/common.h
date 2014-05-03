#define CMD_PIXEL 0

struct pixel {
	int x, y;
	uint8_t bright;
} __attribute__((packed));

struct packet {
	uint8_t cmd;
	union {
		struct pixel pixel;
	};
} __attribute__((packed));
