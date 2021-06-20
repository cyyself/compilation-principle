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
    int n;
    get(n);
    int i = 1;
    while (i <= n) {
        put(fibo(i));
        i = i + 1;
    }
    return 0;
}