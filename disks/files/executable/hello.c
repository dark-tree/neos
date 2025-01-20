
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main() {

	const char* hello = "Hello from The C Programming Language!\n";

	FILE* file = fopen("./hello.txt", "w+");

	fwrite(hello, 1, strlen(hello), file);
	fclose(file);

	_exit(0);

}
