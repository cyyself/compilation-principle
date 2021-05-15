#define DEBUG

#include "grammar_parser.hpp"

#define BUFFER_SIZE 128*1024


char buffer[BUFFER_SIZE];

int main() {
    freopen("../testcase1.c","r",stdin);
    int ptr = -1;
    do {
        ptr ++;
        buffer[ptr] = getchar();
    }
    while (buffer[ptr] != EOF);
    buffer[ptr] = 0; // replace EOF to 0
    grammar_parser parser(buffer);
    parser.parse();
	return 0;
}