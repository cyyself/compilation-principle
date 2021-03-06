#include "compile.hpp"

#define BUFFER_SIZE 128*1024


char buffer[BUFFER_SIZE];

int main() {
#ifdef DEBUG
    freopen("../testcase.c","r",stdin);
#endif
    int ptr = -1;
    do {
        ptr ++;
        buffer[ptr] = getchar();
    }
    while (buffer[ptr] != EOF);
    buffer[ptr] = 0; // replace EOF to 0
    compiler comp(buffer);
    comp.compile();
    comp.output_four_tuple();
    comp.output_mips();
	return 0;
}