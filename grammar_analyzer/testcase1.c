int main() {
    int a,b = 1;
    a = b + 1;
    int c = 2;
    b = b + b;
    bool temp,temp1,temp2;
    {
        temp = (b == c);
        temp = (b > c);
        temp = (b < c);
    }
    {
        temp = (temp1 && temp2);
        temp = (temp1 || temp2);
        temp = (!temp);
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