#define DEBUG
#include "lexical_parser.hpp"

int main() {
    const char *s = "struct c {int a;};int a = 1;int b = 2;struct c d;char *e = \"abcd\"";
    lexical_parser parser;
    parser.parse_lexical(s);
    parser.print_result();
	return 0;
}