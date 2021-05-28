struct DATA {
    int dataa,datab;
    int asdf,qwer;
};
int main() {
    int a,b = 1;
    a = b + 1;
    int c = 2;
    b = b + b;
    {
        a = (b == c);
        a = (b > c);
        a = (b < c);
    }
    {
        a = (b && c);
        a = (b || c);
        a = (!b);
    }
    if (a == b) {
        while (a == b) {
            a ^= b;
        }
    }
    else {
        get(a);
        put(a);
    }
    return 0;
}