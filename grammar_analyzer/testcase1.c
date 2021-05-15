int b;

int add(int x, int y) {
    return x+y;
}

int main() {
    int i;
    int sum = 0;
    for (i=1;i<=10;i++) {
        sum += i;
    }
    int a = 1;
    if (a == 1) {
        ++a;
    }
    else {
        a--;
        if (a == 0) {
            printf("ok");
        }
        else {
            printf("error");
        }
    }
    while (a) {
        a --;
    }
    printf("%d%d",a,a);
}