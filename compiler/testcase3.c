int fibo(int x) {
    int b,t,a;
    a = 1;
    b = 1;
    while (x) {
        t = a + b;
        a = b;
        b = t;
        x = x - 1;
    }
    return a;
}
int main() {
    while (1) {
        int i;
        get(i);
        int f = fibo(i);
        put(f);
    }
    return 0;
}