int add(int x,int y) {
    return x + y;
}
int main() {
    const int a = 123;
    const int b = 456;
    int c = 0;
    c = add(a,b);
    return 0;
}