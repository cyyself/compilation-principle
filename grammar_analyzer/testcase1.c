int main() {
    int a,b = 1;
    a = b + 1;
    b = b + b;
    int c = 2;
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