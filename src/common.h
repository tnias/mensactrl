#define CMD_PIXEL 0
#define CMD_BLIT 1
#define CMD_FILL 2

struct pixel {
	int x, y;
	uint8_t bright;
} __attribute__((packed));

struct blit {
	int x, y;
	int width, height;
	uint8_t data[];
} __attribute__((packed));

struct fill {
	int x, y;
	int width, height;
	uint8_t bright;
} __attribute__((packed));

struct packet {
	uint8_t cmd;
	union {
		struct pixel pixel;
		struct blit blit;
		struct fill fill;
	};
} __attribute__((packed));
