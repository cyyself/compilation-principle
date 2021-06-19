int main() {
    int num1,num2,op,ans;
    get(num1,num2,op);
    if(op==0)
    {
        ans = num1 + num2;
    }
    if(op==1)
    {
        ans = num1 - num2;
    }
    if(op==2)
    {
        ans = num1 & num2;
    }
    if(op==3)
    {
        ans = num1 | num2;
    }
    put(ans);
}