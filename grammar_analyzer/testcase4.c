int main() {
    int i;
    int sum = 0;
    for (i=1;i<=10) {
        sum += i;
    }
    int a = 1;
    if (a == 1) 
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