
volatile const char array[4] = {0x11, 0x22, 0x33, 0x44};

int start() {
	return array[1];
}
