int main() {
    //循环输入op，改变输出结果out，输入0则结束程序
    int num1, num2, num3, out, op;
    num1 = 3333;
    num2 = 6666;
    num3 = 9999;
    op = 1;
    while(op>0)
    {
        if(op==1)
        {
            out = num1;
        }
        if(op==2)
        {
            out = num2;
        }
        if(op==3)
        {
            out = num3;
        }
        put(out);
        get(op);
    }
}