#include "lexical_parser.hpp"
#include "utility"

using std::get;
using std::swap;
using std::pair;
using std::vector;

enum TreeNodeType {
	FUNCTION, // 函数
    SENTENSE, // 语句
	SYMDECLEAR, // 符号的声明，函数、变量、结构体最开始都走向这种声明
    QUALIFIERS, // 修饰符
	SINGLEVAR, // 单个变量的声明
	STRUCTUNION, // 结构体或联合体
	BLOCK, // 代码块
	EXP, // 表达式
	FOR, // for循环
	WHILE, // while循环
	DOWHILE, // do while循环
    OP, // 操作符
    SYM, // 符号
    VAL, // 常量
	TERM //终结符
};

struct TreeNode {
    TreeNode *fa;
	TreeNodeType type;
	pair<int,int> token; //原始的符号，对于OP，使用second存储优先级
	vector <TreeNode*> child;
    TreeNode() {
        fa = NULL;
        token = {-1,0};
    }
};

class grammar_parser {
public:
    grammar_parser(const char *_buffer) {
        buffer = _buffer;
        token = lex.parse_lexical(buffer);
        lex.print_result();
        if (token.size()) {
            sym_id = lex.lexicals.get_lexical_number("sym");
            val_id = lex.lexicals.get_lexical_number("val");
            for (auto x : token) {
                cout << x.first << "\t" << x.second << "\t";
                if (x.first == sym_id) {
                    cout << lex.symbols.get_symbol_str(x.second);
                }
                else if (x.first == val_id) {
                    cout << lex.values.get_value_str(x.second);
                }
                else {
                    cout << lex.lexicals.get_lexical_str(x.first);
                }
                cout << "\n";
            }
        }
        else {
            lex.print_error();
        }
        init_op_priority();
    }
#ifdef DEBUG
    void print_tree(TreeNode *tr,int depth = 0) {
        for (int i=0;i<depth;i++) printf("\t");
        printf("(%d,%d)",tr->token.first,tr->token.second);
        printf("\n");
        for (auto x : tr->child) print_tree(x,depth+1);
    }
    void test() {
        TreeNode *tr;
        parse_exp(0,&tr);
        print_tree(tr);
    }
#endif
private:
    const char* buffer;
    vector <pair<int,int> > token;
    lexical_parser lex;
    int sym_id, val_id;
    map <int,pair<int,bool> > op_priority;
    void init_op_priority() {
        const vector < tuple<int,string,bool> > op_priority_init({
            {1,"[",false},{1,"]",false},{1,".",false},{1,"->",false},
            {2,"!",true},{2,"~",true},{2,"sizeof",true},
            {3,"/",false},{3,"%",false},{3,"*",false},
            {5,"<<",false},{5,">>",false},
            {6,"<",false},{6,"<=",false},{6,">",false},{6,">=",false},
            {7,"==",false},{7,"!=",false},
            {8,"&",false},
            {9,"^",false},
            {10,"|",false},
            {11,"&&",false},
            {12,"||",false},
            {13,"?",true},{13,":",true},
            {14,"=",true},{14,"+=",true},{14,"-=",true},{14,"*=",true},{14,"/=",true},{14,"%=",true},
            {14,"<<=",true},{14,">>=",true},{14,"&=",true},{14,"^=",true},{14,"|=",true}
        });
        for (auto x : op_priority_init) {
            int token_number = lex.lexicals.get_lexical_number(get<1>(x));
            assert(token_number != -1);
            assert(op_priority.find(token_number) == op_priority.end()); // 如果出现相同符号说明语法分析器代码编写有问题
            op_priority[token_number] = {get<0>(x),get<2>(x)};
        }
    }
    
    /*
    function parse_exp: 
    解析表达式

    该TreeNode全为OP或SYM或VOL或FUNCTIONCALL

    OP为二叉树，一个符号连接两个SYM或VOL或OP或单个NULL（对于单结合的符号，例如i++)
    */
    int parse_exp(int start_pos,TreeNode **rt) { // terminal at ,
        int last_op_priority = -1;
        enum token_type {
            NONE,
            VALUE,
            OPERATOR,
        } last_token_type = NONE;
        int token_ptr = start_pos;
        TreeNode *lastnode = NULL;
        while (token_ptr < token.size()) {
            // 不需要特殊处理括号，因为符号表中已完成函数调用的识别，直接使用函数调用的parse函数处理
            // 暂时不对指针进行处理
            string sym_str = lex.lexicals.get_lexical_str(token[token_ptr].first);
            if (sym_str == "(") {
                TreeNode *node;
                int off = parse_exp(token_ptr + 1,&node);
                token_ptr += off;
                sym_str = lex.lexicals.get_lexical_str(token[token_ptr+1].first);
                if (sym_str == ")") {
                    token_ptr += 2;
                }
                else {
                    assert(false);
                }
                if (lastnode == NULL) *rt = lastnode = node;
                else {
                    lastnode->child.push_back(node);
                    lastnode = node;
                }
                last_token_type = VALUE;
            }
            else if (sym_str == "-" || sym_str == "+" || sym_str == "--" || sym_str == "++") {
                if (last_token_type == VALUE && (sym_str == "-" || sym_str == "+")) {
                    // 当做运算符处理
                    int cur_priority = 4;
                    bool cur_as = false;
                    bool dir = false;
                    merge_op(rt,&lastnode,token[token_ptr],last_op_priority,cur_priority,cur_as);
                    last_op_priority = cur_priority;
                    last_token_type = OPERATOR;
                }
                else {
                    // 一元加减处理（正负号）
                    if (last_token_type == VALUE) {
                        // example: i++
                        int cur_priority = 1;
                        TreeNode *node = new TreeNode();
                        node->fa = lastnode->fa;
                        node->type = OP;
                        node->token = {token[token_ptr].first,cur_priority};
                        node->child.push_back(lastnode);
                        node->child.push_back(NULL);
                        if (lastnode->fa) {
                            bool flag = false;
                            for (auto &x : lastnode->fa->child) if (x == lastnode) {
                                x = node;
                                flag = true;
                                break;
                            }
                            assert(flag);
                        }
                        else *rt = node;
                        lastnode->fa = node;
                        lastnode = node;
                        last_op_priority = cur_priority;
                        last_token_type = VALUE;
                    }
                    else {
                        // ++i
                        int cur_priority = 2;
                        TreeNode *node = new TreeNode();
                        node->fa = lastnode;
                        node->type = OP;
                        node->token = {token[token_ptr].first,cur_priority};
                        node->child.push_back(NULL);
                        lastnode->child.push_back(node);
                        lastnode = node;
                        last_op_priority = cur_priority;
                        last_token_type = OPERATOR;
                    }
                }
                token_ptr ++;
            }
            else if (sym_str == "*" || sym_str == "&") { //处理指针与乘号、Bitwise and
                if (last_token_type == VALUE) {
                    // 不当做指针
                    int cur_priority = (sym_str == "*") ? 3 : 8;
                    bool cur_as = false;
                    merge_op(rt,&lastnode,token[token_ptr],last_op_priority,cur_priority,cur_as);
                    last_token_type = OPERATOR;
                    last_op_priority = cur_priority;
                    token_ptr ++;
                }
                else {
                    // 当做指针，右结合
                    int cur_priority = 2;
                    TreeNode *node = new TreeNode();
                    node->fa = lastnode;
                    node->type = OP;
                    node->token = {token[token_ptr].first,cur_priority};
                    node->child.push_back(NULL);
                    lastnode->child.push_back(node);
                    lastnode = node;
                    last_op_priority = cur_priority;
                    last_token_type = OPERATOR;
                }
            }
            else if (op_priority.find(token[token_ptr].first) != op_priority.end()) {
                int cur_priority = op_priority[token[token_ptr].first].first;
                bool cur_as = op_priority[token[token_ptr].first].second;
                merge_op(rt,&lastnode,token[token_ptr],last_op_priority,cur_priority,cur_as);
                last_token_type = OPERATOR;
                last_op_priority = cur_priority;
                token_ptr ++;
            }
            else if (token[token_ptr].first == sym_id) {
                TreeNode *node = new TreeNode();
                node->fa = lastnode;
                node->type = SYM;
                node->token = token[token_ptr];
                if (lastnode == NULL) *rt = lastnode = node;
                else {
                    lastnode->child.push_back(node);
                    lastnode = node;
                }
                last_token_type = VALUE;
                token_ptr ++;
                // TODO: 函数和数组的处理
            }
            else if (token[token_ptr].first == val_id) {
                TreeNode *node = new TreeNode();
                node->fa = lastnode;
                node->type = VAL;
                node->token = token[token_ptr];
                if (lastnode == NULL) *rt = lastnode = node;
                else {
                    lastnode->child.push_back(node);
                    lastnode = node;
                }
                last_token_type = VALUE;
                token_ptr ++;
            }
            else break;
        }
        return token_ptr - start_pos;
    }
    /*
    int parse_function_call(int start_pos) {
        // <func_symbol> <(> <exp><,> ... <)>
        if (start_pos + 1 < token.size() && token[start_pos+1].first == lex.lexicals.get_lexical_number("(")) {
            int off = 2;
            while (true) {
                int tmp = parse_exp(off);
                if (tmp == -1) {
                    return -1; //后续补错误处理
                }
                if (token[start_pos+off+tmp+1].first == lex.lexicals.get_lexical_number(",")) {
                    off += tmp + 1;
                }
                else if (token[start_pos+off+tmp+1].first == lex.lexicals.get_lexical_number(")")) {
                    off += tmp + 1;
                    break;
                }
                else {
                    return -1; //后续补错误处理
                }
            }
            return off;
        }
    }
    */
    int parse_name_with_exp(int start_pos, TreeNode **rt) {

    }
    /*
    在变量声明前读取修饰符，如const、static等，后续加入到符号表定义中。
    */
    int parse_qualifiers(int start_pos, TreeNode **rt) {
        // <修饰符> ::= 空集 | <单个修饰符> <修饰符>
        // 不存在左递归，因此可以使用递归下降
        int token_ptr = start_pos;
        string sym_str = lex.lexicals.get_lexical_str(token[token_ptr].first);
        *rt = new TreeNode();
        (*rt)->type = QUALIFIERS;
        while (token_ptr < token.size() && type_qualifiers.find(sym_str) == type_qualifiers.end()) {
            TreeNode *node = new TreeNode();
            node->fa = *rt;
            node->token = token[token_ptr];
            node->type = SYM;
            (*rt)->child.push_back(node);
            token_ptr ++;
        }
        return token_ptr - start_pos;
    }
    int parse_struct(int start_pos, TreeNode **rt) {
        // 考虑分为结构体的使用和结构体的声明
    }
    int parse_function(int start_pos, TreeNode **rt) {
        // 只考虑函数声明
    }
    int parse_sym_with_array(int start_pos, TreeNode **rt) {
        // sym[exp1][exp2]....
    }
    int parse_symbol_declear(int start_pos, TreeNode **rt) {
        // <符号定义> ::= <修饰符> <类型声明>
        // 其中函数定义的识别已经在词法分析阶段检测大括号的方式预处理，因此这里直接使用当时的结果即可避免递归下降法的回溯
        int token_ptr = start_pos;
        TreeNode *qualifiers;
        token_ptr += parse_qualifiers(token_ptr,&qualifiers);
        string sym_str = lex.lexicals.get_lexical_str(token[token_ptr].first); // 检查类型
        if (types.find(sym_str) != types.end()) {
            if (sym_str == "struct" || sym_str =="union") {
                assert(qualifiers->child.size() == 0);// TODO: 有时间补一下结构体和联合体要求不能有qualifiers
                return parse_struct(token_ptr,rt);
            }
            else {
                int ptr_cnt = 0;
                while (token_ptr + 1 < token.size() && token[token_ptr+1].first == lex.symbols.get_symbol_id("*")) {
                    token_ptr ++;
                    ptr_cnt ++;
                }
                if (token_ptr + 1 < token.size() && token[token_ptr+1].first == sym_id && lex.symbols.has_suffix(token[token_ptr+1].second,"func")) {
                    // 词法分析阶段识别为了函数，按照函数处理
                    // TODO
                }
                else {
                    // 可能是变量或者函数，继续识别
                    // TODO
                }
            }
        }
        else {

        }
    }
    int parse_struct_declear(int start_pos, TreeNode **rt) { // 适用于struct和union的定义
        // TODO
    }
    /*
    merge_op 用于将符号合并到分析树上

    考虑4种情况：
    1. 优先级相同,右结合->直接与lastnode结合
    2. 优先级相同,左结合->一直向上找到相同优先级的最高层，然后结合
    3. 优先级不同且新符号优先级排名更高->直接与lastnode结合
    4. 优先级不同且新符号优先级排名更低->
        4.1. 一直向上找到优先级相同的地方->根据结合性确定链上的位置，若右结合在相等链的最低端，若左结合则在相等链的最顶端
        4.2. 一直向上找到了第一个优先级大于的地方->直接左结合

    传入参数：
        token：当前合并的运算符token
        last_pri：上一个符号的优先级
        cur_pri：当前符号的优先级
        cur_as：当前符号的结合性（C语言中相同优先级结合律一定一致），false为左结合，true为右结合
    */
    void merge_op(TreeNode **rt, TreeNode **lastnode, pair<int,int> token, int last_pri, int cur_pri, bool cur_as) {
        if ((last_pri == cur_pri && cur_as) || last_pri > cur_pri) { // case 1 and case 3
            TreeNode *chnode = new TreeNode();
            swap(**lastnode,*chnode);
            (*lastnode)->fa = chnode->fa;
            (*lastnode)->token = {token.first,cur_pri};
            (*lastnode)->type = OP;
            (*lastnode)->child.push_back(chnode);
            chnode->fa = *lastnode;
        }
        else if (last_pri == cur_pri) { // case 2
            TreeNode *pos = *lastnode;
            while (pos->fa && (pos->fa->type != OP || pos->fa->token.second == cur_pri)) pos = pos->fa;
            TreeNode *chnode = new TreeNode();
            if (pos->fa) {
                bool flag = false;
                for (auto &x : pos->fa->child) if (x == pos) {
                    x = chnode;
                    flag = true;
                    break;
                }
                assert(flag); // debug
            }
            else *rt = chnode;
            chnode->fa = pos->fa;
            pos->fa = chnode;
            chnode->type = OP;
            chnode->token = {token.first,cur_pri};
            chnode->child.push_back(pos);
            *lastnode = chnode;
        }
        else { // case 4 (结合代码一样因此后续合并了）
            TreeNode *pos = *lastnode;
            while (pos->fa && (pos->fa->type != OP || pos->fa->token.second < cur_pri) ) pos = pos->fa;
            if (pos->fa && pos->fa->token.second == cur_pri) { // case 4.1
                // 如果左结合则继续往上走
                if (!cur_as) 
                    while (pos->fa && (pos->fa->type == OP && pos->fa->token.second == cur_pri) )
                        pos = pos->fa;
            }
            TreeNode *chnode = new TreeNode();
            if (pos->fa) {
                bool flag = false;
                for (auto &x : pos->fa->child) if (x == pos) {
                    x = chnode;
                    flag = true;
                    break;
                }
                assert(flag); // debug
            }
            else *rt = chnode;
            chnode->fa = pos->fa;
            pos->fa = chnode;
            chnode->type = OP;
            chnode->token = {token.first,cur_pri};
            chnode->child.push_back(pos);
            *lastnode = chnode;
        }
    }
    void tree_copy(TreeNode *src, TreeNode *dst) {
        dst->token = src->token;
        dst->type = src->type;
        for (auto x : src->child) {
            TreeNode *newnode = new TreeNode();
            dst->child.push_back(newnode);
            newnode->fa = dst;
            tree_copy(x,newnode);
        }
    }
};