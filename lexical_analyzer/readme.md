## 1. 实现的内容

1. 实现了C99标准中的除下划线开头的关键字以外的所有关键字的词法识别

   关键字列表位于[https://en.cppreference.com/w/c/keyword](https://en.cppreference.com/w/c/keyword)

2. 实现了整数、浮点数（含传统`[x].[y]`与指数形式科学计数法`[x].[y]e[z]`）两种形式、字符(char)、字符数组、8进制数、16进制数的常量识别。

3. 实现了识别单行注释`//`与多行注释`/* */`。并在词法分析中可以跳过。

4. 实现了数组与结构体的识别。

5. 实现了统计源程序中的语句行数、各类单词的个数、字符总数，并输出统计结果。

6. 实现了检查源程序中存在的词法错误，并报告错误所在的位置。

7. 实现了对错误进行适当的恢复，跳过错误继续进行词法分析，用一次扫描报告源程序中存在的所有词法错误以及错误位置。

8. 采用了递归下降方法。

## 2. 语言说明

### 词法分析的整体框架

该部分代码位于`src/lexical_parser.hpp`中：

其中，`ptr`为正在读取的代码的指针。

![整体框架](img/整体框架.png)

可以看出，我将这些部分分为了以下3类：

1. 三类词法的识别
   1. 标识符
   2. 运算符
   3. 数值
2. 注释的识别
3. 空白字符的识别（跳过然后保持状态即可）

### 三类词法

该词法分析器将所识别的文本分为**符号**、**数值常量**、**运算符**三类。其中他们的开始字符各不相同，因此我们只需要在读取到不同起始字符的时候进行不一样的处理即可。

- 符号(Symbol)的文法表示：

  在C99标准中，C语言的符号为`_`或**英文字母**作为起始字符。

  之后可包括**数字**、**英文字母**、**下划线**三种文字，也可为空。

  归纳为文法表示如下：

  ```
  <Symbol字符>→<'_'>|<数字>|<英文字母>
  <Symbol起始字符>→<'_'>|<英文字母>
  <Symbol中间部分>→Ɛ|<Symbol字符><Symbol中间部分>
  <Symbol>→<Symbol起始字符>|<Symbol中间部分>
  ```

  这样的形式，在处理完了首字母作为开始字符以后，我们可以用一个简单的C++函数读取出来：

  ```cpp
  int read_keyword(const char *str) { // 由英文字母或者下划线开始后调用这个函数
    int offset = 0;
    while (isdigit(*str) || isalpha(*str) || *str == '_') {
      str ++;
      offset ++;
    }
    return offset; // 返回符号的长度
  }
  ```

- 数值常量的文法表示：

  对于数值常量，我们需要分为以下几类：

  - 十六进制常量：以0x开始的数字。十六进制部分字符包括0...9,a...f

    ```
    <hex字符>→<数字>|<'a'>|<'b'>|<'c'>|<'d'>|<'e'>
    <hex数字部分>→<hex字符>|(Ɛ|<hex数字部分>)
    <十六进制常量>→<'0'><'x'><hex数字部分>
    ```

  - 八进制常量：以0开始的数字，八进制部分字符包括0...7

    ```
    <oct字符>→0...7
    <oct数字部分>→<oct字符>(Ɛ|<oct数字部分>)
    <八进制常量>→<'0'><oct数字部分>
    ```

  - 十进制整数常量：不以0开始的数字，十进制部分字符包括0...9

    ```
    <十进制起始字符>→1...9
    <十进制数中间部分>→Ɛ|(<数字><十进制数中间部分>)
    <十进制整数常量>→<十进制起始字符><十进制数中间部分>
    ```

  - 浮点数常量：形如`123.456`或`123.456e-789`

    ```
    <简单浮点数>→<整数>|(<'.'><非负整数>)
    <浮点数常量>→<简单浮点数>|(<'e'><整数>)
    ```

  - 字符常量：形如`'a'`或`'\a'`（考虑转义符）

    ```
    <字符常量>→'(<'\'><所有ASCII字符>|<除单引号外的所有ASCII字符>)'
    ```

  - 字符数组常量：形如`"Hello World\n"`（也需要考虑转义符）

    ```
    <内部字符串>→Ɛ|((<'\'><所有ASCII字符>|<除单引号外的所有ASCII字符>)|<内部字符串>)
    <字符数组常量>→"<内部字符串>"
    ```

    总结来说，所有的数字开头不外乎就是**数字**、**双引号**`"`、**单引号**`'`，因此我们用C++可以编写以下函数进行解析：

    ```cpp
    enum value_type {
        ERROR_TYPE,INT,FLOAT,STR,OCT,HEX,CHAR
    };
    struct value_result {
        value_type type;
        const char *value;
        int valuelen;
    };
    int parse_value(const char *str,value_result &ret) {
        ret.type = ERROR_TYPE;
        ret.value = str;
        int &offset = ret.valuelen = 0;
        if (str[0] == '\'') { // 检测到字符常量
            ret.type = CHAR;
            if (str[2] == '\'') { // 考虑'a'的形式
                offset = 3;
            }
            else {
                if (str[1] == '\\' && str[3] == '\'') { //考虑'/a'的形式
                    offset = 4;
                }
                else {
                    ret.type = ERROR_TYPE;
                }
            }
            return offset;
        }
        else if (str[0] == '"') { // 检测到字符串（字符数组）常量
            offset ++;
            bool last_zy = false;
            while (str[offset] && str[offset] != EOF && (last_zy || str[offset] != '"')) {
                last_zy = (str[offset] == '\\'); // 考虑转义符，例如\"不应该认为是字符串结束
                offset ++;
            }
            if (str[offset] == EOF || str[offset] == 0) { // 处理读取到终点，防止缓冲区溢出
                ret.type = ERROR_TYPE;
            }
            else {
                offset ++;
                ret.type = STR;
                ret.valuelen = offset;
            }
            return offset;
        }
        else if (isdigit(str[0]) || str[0] == '-') {// 检测到数值常量
            ret.type = INT;
            bool neg = str[0] == '-';
            if (neg) offset ++;
            while (str[offset] && str[offset] != EOF) {
                if (str[offset] == '-') { //根据特殊定位符处理数值类型
                    if (str[offset-1] != 'e') break;
                }
                else if (str[offset] == '.') {
                    ret.type = FLOAT;
                }
                else if (str[offset] == 'e') {
                    ret.type = FLOAT;
                }
                else if (str[offset] == '0' && offset == neg) {
                    ret.type = OCT;
                }
                else if (str[offset] == 'x') {
                    ret.type = HEX;
                }
                else if (!isdigit(str[offset])) {
                    // 如果不是16进制还出现了其它字符，那么就认为可能是到达了边界，直接停止
                    if (!(ret.type == HEX && 
                        (
                            (str[offset] >= 'a' && str[offset] <= 'f') || 
                            (str[offset] >= 'A' && str[offset] <= 'F'))
                        )
                    ) {
                        break;
                    }
                }
                offset ++;
            }
            if (ret.type == OCT) {
                // 考虑八进制数如果出现了别的数字的情况
                for (int i=1;i<offset;i++) if (str[i] == '8' || str[i] == '9') {
                    ret.type = ERROR_TYPE;
                }
            }
            return offset;
        }
        else {
            return 1; // 不可能出现这种情况，但还是返回一个值来跳过这里
        }
    }
    ```

- 运算符的文法表示：

  运算符的识别比较复杂，因为运算符可能存在运算符本身是其它运算符的前缀的情况，例如"<<="与"<<"，这就需要在识别的时候确定确切的运算符起始位置。因此我们考虑先定义运算符列表，然后在后续将匹配过程在**Trie树**（字典树）上进行状态转移。

  Trie树结构如下：（这里只画了以`<`开始的部分，其中蓝色表示普通节点，橙色表示可以识别为符号的节点）

  ![trie](img/trie.png)

  匹配的过程我们只需要逐字读入运算符，然后不断更新在Trie树上最长匹配到的运算符，当树上无法继续转移状态的时候退出函数然后返回匹配到的运算符即可。

  Trie树关键代码如下，位于我的程序中的`src/op_trie.hpp`中的`class opTrie`

  ```cpp
  struct Node {
    Node *ch[128];
    bool exist;
    Node() {
      exist = false;
      for (int i=0;i<128;i++) ch[i] = NULL;
    }
  };
  int query(const char *s) {
    int maxlen = 0;
    Node *cur = root;
    int curlen = 0;
    while (*s) {
      if (*s & 128) break;
      if (cur->ch[*s]) {
        cur = cur->ch[*s];
        curlen ++;
        if (cur->exist) maxlen = curlen;
      }
      else {
        break;
      }
      s ++;
    }
    return maxlen;
  }
  ```

  

  这里我定义的运算符列表如下：

  ```cpp
  std::set <std::string> ops = std::set <std::string> ({
      "+","-","*","/","<",">","==",
      "!=","<<",">>","++","--","&&",
      "||","&","|","^","!","~",">=",
      "<=","?",":","=","+=","-=","*=",
      "/=","<<=",">>=","&=","|=","^=",
      "."
  });
  ```

  基于以上文法表示，我们就可以将代码中需要提取的内容提取出来。

### 注释

之后，我们还需要特殊处理注释，注释处理如下：

- 单行注释

  单行注释的特点是，以`//`为开始（在字符串中的`"//"`不会被作为注释识别），以换行符(`\n`)为结束。

  我们可以采用以下有限状态机(FSM)进行表示：

  ![single_line_comment](img/single_line_comment.png)

- 多行注释

  多行注释的特点是，以`/*`为开始，直到出现`*/`为结束。

  同样可以采用以下FSM表示：

  ![multiple_line_comment](img/multiple_line_comment.png)

## 3. 词类编码表

在词法分析程序中，我采用的方法是先定义各种类型的词法表，然后遍历该表插入到词类编码表中，最后生成结果如下：

| 编码 | 词       | 编码 | 词     | 编码 | 词       | 编码 | 词       |
| ---- | -------- | ---- | ------ | ---- | -------- | ---- | -------- |
| 0    | const    | 1    | extern | 2    | inline   | 3    | register |
| 4    | restrict | 5    | static | 6    | volatile | 7    | auto     |
| 8    | char     | 9    | double | 10   | enum     | 11   | float    |
| 12   | int      | 13   | long   | 14   | short    | 15   | signed   |
| 16   | struct   | 17   | union  | 18   | unsigned | 19   | void     |
| 20   | break    | 21   | case   | 22   | continue | 23   | default  |
| 24   | do       | 25   | else   | 26   | for      | 27   | goto     |
| 28   | if       | 29   | return | 30   | sizeof   | 31   | switch   |
| 32   | typedef  | 33   | while  | 34   | getchar  | 35   | printf   |
| 36   | putchar  | 37   | scanf  | 38   | !        | 39   | !=       |
| 40   | &        | 41   | &&     | 42   | &=       | 43   | *        |
| 44   | *=       | 45   | +      | 46   | ++       | 47   | +=       |
| 48   | -        | 49   | --     | 50   | -=       | 51   | .        |
| 52   | /        | 53   | /=     | 54   | :        | 55   | <        |
| 56   | <<       | 57   | <<=    | 58   | <=       | 59   | =        |
| 60   | ==       | 61   | >      | 62   | >=       | 63   | >>       |
| 64   | >>=      | 65   | ?      | 66   | ^        | 67   | ^=       |
| 68   | \|       | 69   | \|=    | 70   | \|\|     | 71   | ~        |
| 72   | ,        | 73   | ;      | 74   | (        | 75   | )        |
| 76   | [        | 77   | ]      | 78   | {        | 79   | }        |
| 80   | sym      | 81   | val    |      |          |      |          |

其中，最后的"sym"与"val"分别指代用户定义的符号以及用户定义的常量数值。

用于生成词类编码表的词集合如下，位于我的代码中的`src/init_lexical.hpp`

```cpp
std::set <std::string> type_qualifiers = std::set <std::string> ({
    "const","static","volatile","register","inline","extern",
    "restrict"
});
std::set <std::string> types = std::set <std::string>({
    "void","int","long","short","float","double","char",
    "unsigned","signed","struct","union","auto","enum"
});
std::set <std::string> keys = std::set <std::string> ({
    "if","else","goto","switch","case","do","while","for",
    "continue","break","return","default","sizeof","typedef"
});
std::set <std::string> builtin_functions = std::set <std::string> ({
    "getchar","putchar","scanf","printf"
});
std::set <std::string> ops = std::set <std::string> ({
    "+","-","*","/","<",">","==","!=","<<",">>","++","--","&&",
    "||","&","|","^","!","~",">=","<=","?",":","=","+=","-=","*=",
    "/=","<<=",">>=","&=","|=","^=","."
});
std::set <char> control_ops = std::set<char> ({
    ',',';'
});
std::set<std::pair<char,char> > control_ops_pairwise = 
    std::set<std::pair<char,char> >({
        {'{','}'},
        {'(',')'},
        {'[',']'}
    });
```

## 4. 符号表

处理符号表之前，需要先考虑用户自定义符号的识别：

### 变量的定义（含数组）

这里我们规定变量定义文法形式如下：

```
<指针>→Ɛ|(<'*'>|<指针>)
<数组>→Ɛ|(<'['><正整数><']'>|<数组>)
<变量符号串>→(Ɛ|<数组>)<symbol>(Ɛ|<'='><表达式>)|(<','><变量符号串>)
<变量类型>→<C语言预置类型>|((<'struct'>|<'union'>) <'用户定义的类型符号'>)
<变量的定义>→<变量类型> <变量符号串>;
```

这样，形如简单的`int a = 1,b = 2;`或者复杂如`struct DATA *a[12],*b[34]`都可以包含在这样的文法定义中。

### 结构体的定义

```
<多变量的定义>→<变量的定义>|(Ɛ|<多变量的定义>)
<结构体的定义>→<'struct'> <结构体名称(作为Symbol识别)> <'{'> <多变量的定义> <'}'>
```

### 函数的定义

我们规定函数定义文法形式如下：

```
<数组>→Ɛ|(<'['><正整数><']'>|<数组>)
<变量类型>→<C语言预置类型>|((<'struct'>|<'union'>) <'用户定义的类型符号'>)
<单变量的定义>→<变量类型> (Ɛ|<数组>)<symbol>(Ɛ|<'='><表达式>)
<参数的定义>→Ɛ|<单变量的定义>(Ɛ|<,><参数的定义>)
<函数的定义>→<变量类型>|(Ɛ|<'*'>) <函数名(作为Symbol识别)> <'('><参数的定义><')'> (<';'>|<'{'><函数内部内容><'}'>)
```

这样，形如简单的：

```cpp
int sum(int a,int b);
```

或是以下这种：

```cpp
struct result process(struct input *a,struct input *b) {
  // do something
}
```

都可以被我们的文法定义所涵盖识别。

### 代码

结合以上定义，我们可以同时考虑处理定义时的状态转移，编写C++代码如下：

```cpp
// char *ptr为当前读取源代码位置的指针
// string name_of_type 用于临时存储当前正在识别的类型
// bool upd_name_of_type用于记录name_of_type是否更新，若不更新将会在这一步读取完后清空
// pair <string,string> symbol_ready_commit 用于处理结构体和联合体的延迟加入符号表
// string last_def_symbol_name 上次读取到的用于定义的符号名称，用于处理函数的定义加入符号表
if (isalpha(*ptr) || *ptr == '_') { // 识别标识符
    off = read_keyword(ptr); // 递归下降，读取标识符，返回对应标识符的长度
    string tmp = genstring(ptr,off); // 获取标识符，转换为字符串
    if (types.find(tmp) != types.end()) { // 如果当前读取的标识符识别为类型
        name_of_type = tmp; // 更新name_of_type变量，用于后续对函数定义以及
        upd_name_of_type = true; // 设置一个bool变量表名这次更新了name_of_type，在最后不对它进行清空
        lexicals.add_count(lexicals.get_lexical_number(tmp)); // 对于该symbol的统计++，用于最后打印统计结果
        printf("(%02d,NULL)",lexicals.get_lexical_number(tmp)); // 输出该符号二元组
    }
    else {
        if (name_of_type != "") { // 识别为正在定义符号（注意特殊处理struct和union）
            if (name_of_type == "struct" || name_of_type == "union") { // 类型更新为自定义类型
                if (symbols.has_defined(tmp)) { // 如果是一个已经被定义的struct或者union，那么就认为是正在使用该struct或者union的名称
                    lexicals.add_count(lexicals.get_lexical_number("sym"));
                    printf("(%02d,%03d)",lexicals.get_lexical_number("sym"),symbols.get_symbol_id(tmp));
                }
                else { // 这种情况下说明正在定义struct或者union，需要等待识别到大括号{以后再加入符号表
                    symbol_ready_commit = make_pair(name_of_type,tmp); 
                }
                name_of_type = tmp;
                upd_name_of_type = true;
            }
            else {
                if (types.find(name_of_type) != types.end() || symbols.has_struct_or_union(name_of_type)) { // 检测该类型是否已经定义
                    if (lexicals.lexical_exist(tmp)) errors.raise_error(line_number,ptr-linestart,"symbol already defined in the lexicals."); // 检测正在定义的符号是否与先前定义的符号存在冲突
                    else {
                        int inserted_id = symbols.insert(name_of_type+genptr_level(ptr_level),tmp); // 插入符号表
                        lexicals.add_count(lexicals.get_lexical_number("sym"));
                        printf("(%02d,%03d)",lexicals.get_lexical_number("sym"),inserted_id);
                        last_def_symbol_name = tmp;
                        upd_name_of_type = true; // 暂时保留类型，处理逗号之后继续定义的情况
                    }

                }
                else {
                    lexicals.add_count(lexicals.get_lexical_number("sym"));
                    printf("(%02d,?)",lexicals.get_lexical_number("sym"));
                    errors.raise_error(line_number,ptr-linestart,"undefined type \"" + string(tmp) + "\"."); // 检测到没有定义的类型
                }
            }
        }
        else {
            // 直接翻译为对应词法
            if (lexicals.lexical_exist(tmp)) {
                lexicals.add_count(lexicals.get_lexical_number(tmp));
                printf("(%02d,NULL)",lexicals.get_lexical_number(tmp));
            }
            else if (symbols.has_defined(tmp)) {
                lexicals.add_count(lexicals.get_lexical_number("sym"));
                printf("(%02d,%03d)",lexicals.get_lexical_number("sym"),symbols.get_symbol_id(tmp));
            }
            else {
                printf("(?,?)");
                errors.raise_error(line_number,ptr-linestart,string("undefined symbol \"") + tmp + string("\"."));
            }
        }
    }
}
```

而对于函数定义以及结构体定义，我们相应地需要延迟更新，这里考虑两种字符。函数定义后一定存在`(`，而结构体定义后一定存在`{`，因此就可以在读取到对应字符的时候完成延迟更新操作。

- 对于函数定义的延迟加入符号表：

  在读取到`(`时进行

  ```cpp
  if (tmp == "(") {
    if (last_def_symbol_name != "") {
      symbols.append_type(last_def_symbol_name," func");
    }
    // ... 省略其它对于(的处理
  }
  ```

- 对于结构体的定义延迟加入符号表：

  在读取到`{`时进行

  ```cpp
  if (tmp == "{") {
    if (symbol_ready_commit != make_pair(string(""),string(""))) {
      if (lexicals.lexical_exist(symbol_ready_commit.second)) errors.raise_error(line_number,ptr-linestart,"symbol already defined in the lexicals.");
      else {
        int inserted_id = symbols.insert(symbol_ready_commit.first,symbol_ready_commit.second);
        lexicals.add_count(lexicals.get_lexical_number("sym"));
        printf("(%02d,%03d,%s)",lexicals.get_lexical_number("sym"),inserted_id,symbol_ready_commit.second.c_str());
      }
      symbol_ready_commit = make_pair(string(""),string(""));
    }
    // ... 省略其它对于{的处理
  }
  ```

### 符号表的输出

对于提交的测试程序`testcase1.c`，符号表统计输出结果如下：

![symbol](img/symbol.png)

可以看到，这里正确处理了结构体、数组、函数的识别。

## 5. 错误处理

未完待续