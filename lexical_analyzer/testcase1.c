#include <stdio.h>

struct DATA {
    int dataa, datab;
};


int add(int a,int b) {
    return a + b;
}


int main() {
    const char *s = "Hello world!";
    int i = 0;
    while (s[i])
        putchar(s[i++]);
    struct DATA c;
    c.dataa = 1;
    c.datab = 02;
    float test_f = 0.123 + 1e-1;
    int d = add(123,456);
    if (d == 0x243 && c.dataa == 1) {
        printf("correct\n");
    }
    else {
        printf("incorrect\n");
    }
    int array[123][456];
    return 0;
}
