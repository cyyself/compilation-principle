#include "lexical_parser.hpp"
#include "utility"

using std::get;
using std::swap;
using std::pair;
using std::vector;

enum TreeNodeType {
	FUNCTION, // 函数
    SENTENSE, // 语句
	VAR, // 变量声明
	SINGLE_VAR, // 单个变量的声明
	STRUCT, // 结构体
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
    void pause() {

    }
    void test() {
        TreeNode *tr;
        parse_exp(0,&tr);
        pause();
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
            {1,"[",false},
            {1,"]",false},
            {1,".",false},
            {1,"->",false},
            {2,"!",true},
            {2,"~",true},
            {2,"sizeof",true},
            {3,"/",false},
            {3,"%",false},
            {3,"*",false},
            {5,"<<",false},
            {5,">>",false},
            {6,"<",false},
            {6,"<=",false},
            {6,">",false},
            {6,">=",false},
            {7,"==",false},
            {7,"!=",false},
            {8,"&",false},
            {9,"^",false},
            {10,"|",false},
            {11,"&&",false},
            {12,"||",false},
            {13,"?",true},
            {13,":",true},
            {14,"=",true},
            {14,"+=",true},
            {14,"-=",true},
            {14,"*=",true},
            {14,"/=",true},
            {14,"%=",true},
            {14,"<<=",true},
            {14,">>=",true},
            {14,"&=",true},
            {14,"^=",true},
            {14,"|=",true}
        });
        for (auto x : op_priority_init) {
            int token_number = lex.lexicals.get_lexical_number(get<1>(x));
            assert(token_number != -1);
            assert(op_priority.find(token_number) == op_priority.end()); // 如果出现相同符号说明语法分析器代码编写有问题
            op_priority[token_number] = {get<0>(x),get<2>(x)};
        }
    }
    
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
                    token_ptr ++;
                }
                else {
                    assert(false);
                }
                last_token_type = VALUE;
            }
            else if (sym_str == "-" || sym_str == "+" || sym_str == "--" || sym_str == "++") {
                if (last_token_type == VALUE) {
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
                    TreeNode *node = new TreeNode();
                    node->fa = lastnode;
                    node->type = OP;
                    node->token = token[token_ptr];
                    node->child.push_back(NULL);
                    if (lastnode == NULL) *rt = lastnode = node;
                    else lastnode->child.push_back(node);
                    last_token_type = VALUE;
                }
                token_ptr ++;
            }
            else if (op_priority.find(token[token_ptr].first) != op_priority.end()) {
                int cur_priority = op_priority[token[token_ptr].first].first;
                bool cur_as = op_priority[token[token_ptr].first].second;
                bool dir = false;
                if (cur_priority > last_op_priority) dir = true;
                else if (cur_priority == last_op_priority && !cur_as) dir = true;
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
                assert(flag = true); // debug
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
                assert(flag = true); // debug
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
};