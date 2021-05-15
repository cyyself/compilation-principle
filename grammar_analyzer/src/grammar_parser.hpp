#include "lexical_parser.hpp"
#include "utility"

using std::get;
using std::swap;
using std::pair;
using std::vector;

enum TreeNodeType {
	FUNCTION_CALL, // 函数调用符 函数名(参数,参数2,...)
    FUNCTION_DECLARE, // 函数定义，token为函数本身符号名，child顺序是 修饰符,类型,参数(VARDECLEAERS),代码块，其中，类型的token.second为指针级数
    RETURN, // return 语句
    QUALIFIERS, // 修饰符
    VARDECLEAERS, // 一组的变量声明
    SINGLEQUALIFIER, // 单个修饰符
    SINGLETYPEVAR, // 单个变量的声明，token.first为-1，token.second表示指针级数，child顺序是 修饰符,类型,初始值
	SINGLEVAR, // 单个变量的声明或使用（包含数组）（不含类型以及修饰符) 本身token表示sym，child依次放入数组
    TYPE, // 定义类型，如果是结构体则先为struct/union则先为struct/union对应的符号，
	BLOCK, // 代码块
	EXP, // 表达式
    IF, // if
	FOR, // for循环
	WHILE, // while循环
    OP, // 操作符，保证一定二叉，token.second表示操作符优先级，对于单结合符号例如指针等存在一个NULL
    SYM, // 符号
    VAL, // 常量
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
    void append_ch(TreeNode *node) {
        if (node) {
            assert(node->fa == NULL); // 避免多连接问题
            node->fa = this;
        }
        child.push_back(node);
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
        if (tr) {
            printf("(%d,%d)\n",tr->token.first,tr->token.second);
            for (auto x : tr->child) print_tree(x,depth+1);
        }
        else {
            printf("NULL\n");
        }
    }
    void test() {
        TreeNode *tr;
        parse_codeblock(0,&tr);
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
            {1,".",false},{1,"->",false}, // 数组在处理符号的时候单独处理
            {2,"!",true},{2,"~",true},{2,"sizeof",true},
            {3,"/",false},{3,"%",false},
            {5,"<<",false},{5,">>",false},
            {6,"<",false},{6,"<=",false},{6,">",false},{6,">=",false},
            {7,"==",false},{7,"!=",false},
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

    该TreeNode全为OP或SYM或VAL或FUNCTION_CALL

    OP为二叉树，一个符号连接两个SYM或VAL或OP或单个NULL（对于单结合的符号，例如i++)
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
                        if (lastnode == NULL) *rt = lastnode = node;
                        else {
                            lastnode->child.push_back(node);
                            lastnode = node;
                        }
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
                    if (lastnode == NULL) *rt = lastnode = node;
                    else {
                        lastnode->child.push_back(node);
                        lastnode = node;
                    }
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
                if (lex.symbols.has_suffix(token[token_ptr].second,"func")) {
                    TreeNode *node;
                    token_ptr += parse_function_with_param(token_ptr,&node);
                    if (lastnode == NULL) *rt = lastnode = node;
                    else {
                        lastnode->child.push_back(node);
                        lastnode = node;
                    }
                }
                else {
                    TreeNode *node;
                    token_ptr += parse_sym_with_array(token_ptr,&node);
                    if (lastnode == NULL) *rt = lastnode = node;
                    else {
                        lastnode->child.push_back(node);
                        lastnode = node;
                    }
                }
                last_token_type = VALUE;
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
    在变量声明前读取修饰符，如const、static等，后续加入到符号表定义中。
    */
    int parse_qualifiers(int start_pos, TreeNode **rt) {
        // <修饰符> ::= 空集 | (<单个修饰符> <修饰符>)
        // 不存在左递归，因此可以使用递归下降
        int token_ptr = start_pos;
        string sym_str = lex.lexicals.get_lexical_str(token[token_ptr].first);
        *rt = new TreeNode();
        (*rt)->type = QUALIFIERS;
        while (token_ptr < token.size() && type_qualifiers.find(sym_str) != type_qualifiers.end()) {
            TreeNode *node = new TreeNode();
            node->token = token[token_ptr];
            node->type = SINGLEQUALIFIER;
            (*rt)->append_ch(node);
            token_ptr ++;
        }
        return token_ptr - start_pos;
    }
    int parse_function_with_param(int start_pos, TreeNode **rt) {
        assert(token[start_pos].first == sym_id && lex.symbols.has_suffix(token[start_pos].second,"func"));
        *rt = new TreeNode();
        (*rt)->type = FUNCTION_CALL;
        (*rt)->token = token[start_pos];
        int token_ptr = start_pos;
        token_ptr ++;
        if (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == "(") {
            token_ptr ++;
            while (lex.lexicals.get_lexical_str(token[token_ptr].first) != ")") {
                TreeNode *node;
                int off = parse_exp(token_ptr,&node);
                token_ptr += off;
                (*rt)->append_ch(node);
                if (lex.lexicals.get_lexical_str(token[token_ptr].first) == ",") {
                    token_ptr ++;
                }
                else if (lex.lexicals.get_lexical_str(token[token_ptr].first) != ")") {
                    goto err;
                }
            }
            token_ptr ++;
        }
        else goto err;
        return token_ptr - start_pos;
        err:
            assert(false);
    }
    int parse_if(int start_pos, TreeNode **rt) {
        assert(lex.lexicals.get_lexical_str(token[start_pos].first) == "if");
        *rt = new TreeNode();
        (*rt)->type = IF;
        (*rt)->token = token[start_pos];
        int token_ptr = start_pos;
        token_ptr ++;
        if (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == "(") {
            token_ptr ++;
            TreeNode *node;
            int off = parse_exp(token_ptr,&node);
            token_ptr += off;
            (*rt)->append_ch(node);
            if (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == ")") {
                token_ptr ++;
                if (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == "{") {
                    token_ptr ++;
                    TreeNode *node;
                    int off = parse_codeblock(token_ptr,&node);
                    (*rt)->append_ch(node);
                    token_ptr += off;
                    if (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == "}") {
                        token_ptr ++;
                        if (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == "else") {
                            token_ptr ++;
                            if (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == "{") {
                                token_ptr ++;
                                TreeNode *node;
                                int off = parse_codeblock(token_ptr,&node);
                                (*rt)->append_ch(node);
                                token_ptr += off;
                                if (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == "}") {
                                    token_ptr ++;
                                    return token_ptr - start_pos;
                                }
                                else {
                                    assert(false); // TODO: 错误处理
                                }
                            }
                            else {
                                assert(false); // TODO: 错误处理
                            }
                        }
                        else return token_ptr - start_pos;
                    }
                }
            }
            else {
                assert(false); // TODO: 错误处理
            }
        }
        else {
            assert(false); // TODO: 错误处理
        }
        return token_ptr - start_pos;
    }
    int parse_while(int start_pos, TreeNode **rt) {
        assert(lex.lexicals.get_lexical_str(token[start_pos].first) == "while");
        *rt = new TreeNode();
        (*rt)->type = WHILE;
        (*rt)->token = token[start_pos];
        int token_ptr = start_pos;
        token_ptr ++;
        if (token_ptr < token.size() && token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == "(") {
            token_ptr ++;
            TreeNode *node;
            int off = parse_exp(token_ptr,&node);
            token_ptr += off;
            (*rt)->append_ch(node);
            if (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == ")") {
                token_ptr ++;
                if (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == "{") {
                    token_ptr ++;
                    TreeNode *node;
                    int off = parse_codeblock(token_ptr,&node);
                    (*rt)->append_ch(node);
                    token_ptr += off;
                    if (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == "}") {
                        token_ptr ++;
                        return token_ptr - start_pos;
                    }
                }
            }
            else {
                assert(false); // TODO: 错误处理
            }
        }
        else {
            assert(false); // TODO: 错误处理
        }
        return token_ptr - start_pos;
    }
    int parse_return(int start_pos, TreeNode **rt) {
        int token_ptr = start_pos;
        assert(token[token_ptr].first == lex.lexicals.get_lexical_number("return"));
        (*rt) = new TreeNode();
        (*rt)->type = RETURN;
        (*rt)->token = token[token_ptr];
        token_ptr ++;
        TreeNode *exp;
        token_ptr += parse_exp(token_ptr,&exp);
        (*rt)->append_ch(exp);
        if (token[token_ptr].first == lex.lexicals.get_lexical_number(";")) {
            token_ptr ++;
            return token_ptr - start_pos;
        }
        else {
            // TODO: 错误处理
            assert(false);
        }
        return token_ptr - start_pos;
    }
    int parse_sentense(int start_pos, TreeNode **rt) {
        string sym_str = lex.lexicals.get_lexical_str(token[start_pos].first);
        if (types.find(sym_str) != types.end() || type_qualifiers.find(sym_str) != type_qualifiers.end()) {
            // 按照类型处理
            int off = parse_symbol_declare(start_pos,rt);
            return off;
        }
        else if (sym_str == "if") {
            int off = parse_if(start_pos,rt);
            return off;
        }
        else if (sym_str == "for") {
            assert(false); // TODO
        }
        else if (sym_str == "while") {
            int off = parse_while(start_pos,rt);
            return off;
        }
        else if (sym_str == "return") {
            int off = parse_return(start_pos,rt);
            return off;
        }
        else {
            int off = parse_exp(start_pos,rt);
            if (token[start_pos + off].first == lex.lexicals.get_lexical_number(";")) {
                return off + 1;
            }
            else {
                assert(false); // TODO: 错误处理
            }
        }
    }
    int parse_codeblock(int start_pos, TreeNode **rt) {
        int token_ptr = start_pos;
        *rt = new TreeNode();
        (*rt)->type = BLOCK;
        while (token_ptr < token.size() && token[token_ptr].first != lex.lexicals.get_lexical_number("}")) {
            TreeNode *node;
            int off = parse_sentense(token_ptr,&node);
            if (off == 0) {
                assert(false); // TODO: 错误处理
            }
            else {
                token_ptr += off;
                (*rt)->append_ch(node);
            }
        }
        return token_ptr - start_pos;
    }
    int parse_sym_with_array(int start_pos, TreeNode **rt) {
        // sym[exp1][exp2]....
        int token_ptr = start_pos;
        if (token[start_pos].first == sym_id && !lex.symbols.has_suffix(token[token_ptr].second,"func")) {
            *rt = new TreeNode();
            (*rt)->type = SINGLEVAR;
            (*rt)->token = token[start_pos];
            token_ptr ++;
            while (token_ptr < token.size() && token[token_ptr].first == lex.lexicals.get_lexical_number("[")) {
                // 检测到数组声明
                token_ptr ++;
                TreeNode *exp;
                int off = parse_exp(token_ptr,&exp);
                token_ptr += off;
                if (token_ptr < token.size() && token[token_ptr].first == lex.lexicals.get_lexical_number("]")) {
                    (*rt)->append_ch(exp);
                    token_ptr ++;
                }
                else {
                    assert(false);
                    // TODO: 错误处理
                }
            }
        }
        else {
            assert("false"); // 说明语法分析器有问题
        }
        return token_ptr - start_pos;
    }
    int parse_var_declare(int start_pos, TreeNode **rt, TreeNode *qualifiers, bool allow_multiple = true) { // allow_multiple用于识别函数参数，因为采用的是逗号分隔
        // 注意：这里不包含修饰符，修饰符作为参数传入
        // example: int *sym1[exp1][exp2], sym2 = exp2;
        // 这里不处理struct/union，这两者用单独函数处理
        // SINGLETYPEVAR: 单个变量的声明，包含类型和修饰符以及指针 其中token.second表示指针级数，child顺序是 修饰符,类型,SINGLEVAR(包含符号名和数组),初始值(optional)
        int token_ptr = start_pos;
        string sym_str = lex.lexicals.get_lexical_str(token[token_ptr].first);
        if (types.find(sym_str) != types.end() && sym_str != "struct" && sym_str != "union") {
            if (allow_multiple) {
                (*rt) = new TreeNode();
                (*rt)->type = VARDECLEAERS;
            }
            pair <int,int> type_token = token[token_ptr];
            token_ptr ++;
            do {
                if (allow_multiple && lex.lexicals.get_lexical_str(token[token_ptr].first) == ";") {
                    token_ptr ++;
                    break;
                }
                int ptr_cnt = 0;
                while (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == "*") { // 检测到指针
                    ptr_cnt ++;
                    token_ptr ++;
                }
                if (token_ptr < token.size() && token[token_ptr].first == sym_id) {
                    TreeNode *single_var;
                    int off = parse_sym_with_array(token_ptr,&single_var); // TODO: 错误处理
                    token_ptr += off;
                    TreeNode *thisnode = new TreeNode();
                    thisnode->type = SINGLETYPEVAR;
                    thisnode->token = {-1,ptr_cnt};

                    if (qualifiers) {
                        TreeNode *tmpqualifiers = new TreeNode();
                        tree_copy(qualifiers,tmpqualifiers);
                        thisnode->append_ch(tmpqualifiers);
                    }
                    else {
                        thisnode->append_ch(NULL);
                    }

                    TreeNode *type = new TreeNode();
                    type->type = TYPE;
                    type->token = type_token;
                    thisnode->append_ch(type);

                    thisnode->append_ch(single_var);


                    if (token_ptr < token.size() && token[token_ptr].first == lex.lexicals.get_lexical_number("=")) { // 处理初始化
                        token_ptr ++;
                        TreeNode *value;
                        int off = parse_exp(token_ptr,&value);
                        token_ptr += off;
                        thisnode->append_ch(value);
                    }
                    if (allow_multiple) {
                        (*rt)->append_ch(thisnode);
                        if (token_ptr < token.size() && token[token_ptr].first == lex.lexicals.get_lexical_number(",")) {
                            token_ptr ++;
                        }
                    }
                    else {
                        *rt = thisnode;
                        return token_ptr - start_pos;
                    }
                }
                else {
                    break;
                }
            }
            while (allow_multiple);
        }
        else {
            assert(false); // 那么程序不应该走到这里，属于程序本身错误
        }
        return token_ptr - start_pos;
    }
    int parse_function_declare(int start_pos, TreeNode **rt, TreeNode *qualifiers) {
        int token_ptr = start_pos;
        string sym_str = lex.lexicals.get_lexical_str(token[token_ptr].first);
        if (types.find(sym_str) != types.end() && sym_str != "struct" && sym_str != "union") {
            (*rt) = new TreeNode();
            (*rt)->type = FUNCTION_DECLARE;
            pair <int,int> type_token = token[token_ptr];
            token_ptr ++;
            int ptr_cnt = 0;
            while (token_ptr < token.size() && lex.lexicals.get_lexical_str(token[token_ptr].first) == "*") { // 检测到指针
                ptr_cnt ++;
                token_ptr ++;
            }
            type_token.second = ptr_cnt;
            assert(token_ptr < token.size() && token[token_ptr].first == sym_id);
            if (qualifiers) {
                TreeNode *tmpqualifiers = new TreeNode();
                tree_copy(qualifiers,tmpqualifiers);
                (*rt)->append_ch(tmpqualifiers);
            }
            else {
                (*rt)->append_ch(NULL);
            }

            TreeNode *type = new TreeNode();
            type->type = TYPE;
            type->token = type_token;
            (*rt)->append_ch(type);

            if (token[token_ptr].first == sym_id && lex.symbols.has_suffix(token[token_ptr].second,"func")) {
                (*rt)->token = token[token_ptr];
                token_ptr ++;
                if (token[token_ptr].first == lex.lexicals.get_lexical_number("(")) {
                    token_ptr ++;
                    TreeNode *params = new TreeNode();
                    params->type = VARDECLEAERS;
                    while (token_ptr < token.size() && token[token_ptr].first != lex.lexicals.get_lexical_number(")")) {
                        TreeNode *param_qualifiers;
                        token_ptr += parse_qualifiers(token_ptr,&param_qualifiers);
                        TreeNode *var_declare;
                        token_ptr += parse_var_declare(token_ptr,&var_declare,param_qualifiers,false);
                        params->append_ch(var_declare);
                        if (token[token_ptr].first == lex.lexicals.get_lexical_number(",")) token_ptr ++;
                    }
                    (*rt)->append_ch(params);
                    token_ptr ++;
                    if (token[token_ptr].first == lex.lexicals.get_lexical_number("{")) {
                        token_ptr ++;
                        TreeNode *codeblock;
                        token_ptr += parse_codeblock(token_ptr,&codeblock);
                        (*rt)->append_ch(codeblock);
                        if (token[token_ptr].first == lex.lexicals.get_lexical_number("}")) {
                            token_ptr ++;
                            return token_ptr - start_pos;
                        }
                        else goto err;
                    }
                    else goto err;
                }
                else goto err;
            }
            else assert(false); // 那么程序不应该走到这里，属于程序本身错误
            // TODO: 这个函数写一半
        }
        else {
            assert(false); // 那么程序不应该走到这里，属于程序本身错误
        }
        return token_ptr - start_pos;
    err:
        assert(false);
    }
    int parse_symbol_declare(int start_pos, TreeNode **rt) {
        // <符号定义> ::= <修饰符> <类型声明>
        // 其中函数定义的识别已经在词法分析阶段检测大括号的方式预处理，因此这里直接使用当时的结果即可避免递归下降法的回溯
        int token_ptr = start_pos;
        TreeNode *qualifiers;
        token_ptr += parse_qualifiers(token_ptr,&qualifiers);
        string sym_str = lex.lexicals.get_lexical_str(token[token_ptr].first); // 检查类型
        if (types.find(sym_str) != types.end()) {
            if (sym_str == "struct" || sym_str =="union") {
                assert(false); // 不支持的语法
            }
            else {
                int ptr_checkpoint = token_ptr;
                int ptr_cnt = 0;
                while (token_ptr + 1 < token.size() && token[token_ptr+1].first == lex.symbols.get_symbol_id("*")) {
                    token_ptr ++;
                    ptr_cnt ++;
                }
                if (token_ptr + 1 < token.size() && token[token_ptr+1].first == sym_id && lex.symbols.has_suffix(token[token_ptr+1].second,"func")) {
                    return parse_function_declare(ptr_checkpoint,rt,qualifiers);
                    // 词法分析阶段识别为了函数，按照函数处理
                }
                else {
                    // 识别为变量，继续处理
                    return parse_var_declare(ptr_checkpoint,rt,qualifiers);
                }
            }
        }
        else {
            assert(false); // 这种情况属于程序错误
        }
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
        if (src) {
            dst = new TreeNode();
        }
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