#define DEBUG
#include "lexical_parser.hpp"
#define BUFFER_SIZE 128*1024
char buffer[BUFFER_SIZE];
int main() {
    int ptr = -1;
    do {
        ptr ++;
        buffer[ptr] = getchar();
    }
    while (buffer[ptr] != EOF);
    // const char *buffer = "struct c {int a;};int a = 1;int b = 2;struct c d;char *e = \"abcd\"";
    lexical_parser parser;
    parser.parse_lexical(buffer);
    parser.print_result();
	return 0;
}