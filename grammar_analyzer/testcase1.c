int b;

int add(int x, int y) {

}

int main() {
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