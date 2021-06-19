int fibo(int x) {
    int a,b,t;
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
    int f;
    get(f);
    f = fibo(f);
    put(f);
    return 0;
}