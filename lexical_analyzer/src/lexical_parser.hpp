#include "init_lexical.hpp"
#include "value_parser.hpp"
#include "op_trie.hpp"
#include "error_manager.hpp"
#include "value_manager.hpp"
#include "lexical_manager.hpp"
#include "symbol_manager.hpp"

#include <iostream>
#include <cctype>
#include <string>
#include <map>
#include <vector>
#include <utility>

using std::cout;

using namespace value_parser;

class lexical_parser {
public:
    void parse_lexical(const char *s) {
        errors.init_lines(s);
        const char *ptr = s;
        bool comment_single = false, comment_multiline = false; // 注释识别状态机
        int line_number = 1; // 当前的行号
        const char* linestart = ptr;
        bool lastistype = false;
        string name_of_type; // 当前的类型，用于进行类型的识别
        string last_def_symbol_name; // 上次读取到的用于定义的符号名称
        int br_round = 0, br_curly = 0, br_square = 0; // 处理括号匹配
        int last_curly_line = -1, last_curly_col = -1; // 输出大括号匹配错误信息
        pair <string,string> symbol_ready_commit; // 用于识别结构体和联合体定义，需要等待识别到对应的定义字符才提交更改
        int ptr_level = 0; // 用于识别多级指针
        while (*ptr != EOF) {
            bool upd_name_of_type = false; // 处理上次类型是否更新，如果未更新则在最后清空name of type
            if (*ptr == '\n') { // 处理换行
                printf("\n");
                line_number ++;
                linestart = ptr;
                if (comment_single) { // 处理多行注释
                    comment_single = false;
                }
                ptr ++;
            }
            else { // 其它字符
                if (comment_multiline || comment_single) { // 位于注释状态中
                    if (*ptr == '*' && *(ptr+1) == '/') {
                        comment_multiline = false; // 处理多行注释结束
                        ptr += 2;
                    }
                    else ptr ++;
                }
                else {
                    int off = 0;
                    if ((*ptr == '/' && *(ptr+1) == '/') || *ptr == '#') { // 单行注释以及#跳过处理
                        comment_single = true;
                        off = 1 + (*ptr == '/');
                    }
                    else if (*ptr == '/' && *(ptr+1) == '*')  { // 多行注释
                        comment_multiline = true;
                        off = 2;
                    }
                    else if (*ptr == '\t' || *ptr == '\r' || *ptr == ' ') { // 保持状态
                        // 保持缩进，方便阅读
#ifndef NO_SRC_MODE
                        if (*ptr == '\t') printf("\t");
                        else if(*ptr == ' ') printf(" ");
#endif
                        upd_name_of_type = true;
                        off = 1;
                    }
                    else if (isalpha(*ptr) || *ptr == '_') { // 识别标识符
                        off = read_keyword(ptr);
                        string tmp = genstring(ptr,off);
                        if (types.find(tmp) != types.end()) { // 识别为类型
                            name_of_type = tmp;
                            upd_name_of_type = true;
#ifndef NO_SRC_MODE
                            printf("(%02d,NULL,\"%s\")",lexicals.get_lexical_number(tmp),tmp.c_str());
#else
                            printf("(%02d,NULL)",lexicals.get_lexical_number(tmp));
#endif
                        }
                        else {
                            if (name_of_type != "") { // 识别为正在定义符号（注意特殊处理struct和union）
                                if (name_of_type == "struct" || name_of_type == "union") { // 类型更新为自定义类型
                                    if (symbols.has_defined(tmp)) {
#ifndef NO_SRC_MODE
                                        printf("(%02d,%03d,\"%s\")",lexicals.get_lexical_number("sym"),symbols.get_symbol_id(tmp),tmp.c_str());
#else
                                        printf("(%02d,%03d)",lexicals.get_lexical_number("sym"),symbols.get_symbol_id(tmp));
#endif
                                    }
                                    else {
                                        symbol_ready_commit = make_pair(name_of_type,tmp); // 需要等待定义结束再commit
                                    }
                                    name_of_type = tmp;
                                    upd_name_of_type = true;
                                }
                                else {
                                    if (types.find(name_of_type) != types.end() || symbols.has_struct_or_union(name_of_type)) {
                                        int inserted_id = symbols.insert(name_of_type+genptr_level(ptr_level),tmp);
#ifndef NO_SRC_MODE
                                        printf("(%02d,%03d,\"%s\")",lexicals.get_lexical_number("sym"),inserted_id,tmp.c_str());
#else
                                        printf("(%02d,%03d)",lexicals.get_lexical_number("sym"),inserted_id);
#endif
                                        last_def_symbol_name = tmp;
                                        upd_name_of_type = true; // 暂时保留类型，处理逗号之后继续定义的情况
                                    }
                                    else {
#ifndef NO_SRC_MODE
                                        printf("(%02d,?,\"%s\")",lexicals.get_lexical_number("sym"),tmp.c_str());
#else
                                        printf("(%02d,?)",lexicals.get_lexical_number("sym"));
#endif
                                        errors.raise_error(line_number,ptr-linestart,"undefined type \"" + string(tmp) + "\".");
                                    }
                                }
                            }
                            else {
                                // 直接翻译为对应词法
                                if (lexicals.lexical_exist(tmp)) {
#ifndef NO_SRC_MODE
                                    printf("(%02d,NULL,\"%s\")",lexicals.get_lexical_number(tmp),tmp.c_str());
#else
                                    printf("(%02d,NULL)",lexicals.get_lexical_number(tmp));
#endif
                                }
                                else if (symbols.has_defined(tmp)) {
#ifndef NO_SRC_MODE
                                    printf("(%02d,%03d,\"%s\")",lexicals.get_lexical_number("sym"),symbols.get_symbol_id(tmp),tmp.c_str());
#else
                                    printf("(%02d,%03d)",lexicals.get_lexical_number("sym"),symbols.get_symbol_id(tmp));
#endif
                                }
                                else {
#ifndef NO_SRC_MODE
                                    printf("(?,?,\"%s\")",tmp.c_str());
#else
                                    printf("(?,?)");
#endif
                                    errors.raise_error(line_number,ptr-linestart,string("undefined symbol \"") + tmp + string("\"."));
                                }
                            }
                        }
#ifdef DEBUG
                        cout << tmp << "\n";
#endif
                    }
                    else if (trie.firstExist(*ptr)) { // 识别符号
                        off = trie.query(ptr);
                        string tmp = genstring(ptr,off);
                        if (off == 0) {
                            // 无法识别为任何符号的模式
                            errors.raise_error(line_number,ptr-linestart,"operator decode error");
                            off = 1;
                        }
                        else {
                            if (tmp == "*" || tmp == ",") {
                                if (tmp == "*") { // 处理指针
                                    ptr_level ++;
                                }
                                else if (tmp == ",") {
                                    ptr_level = 0;
                                }
                                upd_name_of_type = true; // 如果定义的是函数指针或者逗号，需要保留之前所定义的类型。
                            }
                            else if (tmp == "(") { // 处理括号匹配 {
                                if (last_def_symbol_name != "") {
                                    symbols.append_type(last_def_symbol_name," func");
                                }
                                br_round ++;
                            }
                            else if (tmp == ")") {
                                br_round --;
                                if (br_round < 0) errors.raise_error(line_number,ptr-linestart,"bracket didn't match.");
                            }
                            else if (tmp == "{") {
                                br_curly ++;
                                if (symbol_ready_commit != make_pair(string(""),string(""))) {
                                    int inserted_id = symbols.insert(symbol_ready_commit.first,symbol_ready_commit.second);
                                    printf("(%02d,%03d,%s)",lexicals.get_lexical_number("sym"),inserted_id,symbol_ready_commit.second.c_str());
                                    symbol_ready_commit = make_pair(string(""),string(""));
                                }
                                last_curly_line = line_number;
                                last_curly_col = ptr - linestart;
                            }
                            else if (tmp == "}") {
                                br_curly --;
                                if (br_curly < 0) errors.raise_error(line_number,ptr-linestart,"bracket didn't match.");
                                last_curly_line = line_number;
                                last_curly_col = ptr - linestart;
                            }
                            else if (tmp == "[") {
                                upd_name_of_type = true;
                                br_square ++;
                            }
                            else if (tmp == "]") {
                                upd_name_of_type = true;
                                br_square --;
                                if (br_square < 0) errors.raise_error(line_number,ptr-linestart,"bracket didn't match.");
                            }
                            else if (tmp == ";") { // 处理括号匹配 }
                                ptr_level = 0;
                                symbol_ready_commit = make_pair(string(""),string(""));
                                if (br_round != 0 || br_square != 0) {
                                    errors.raise_error(line_number,ptr-linestart,"bracket didn't match when sentense ends.");
                                    br_round = 0;
                                    br_square = 0;
                                }
                            }
#ifndef NO_SRC_MODE
                            printf("(%02d,NULL,\"%s\")",lexicals.get_lexical_number(tmp),tmp.c_str());
#else
                            printf("(%02d,NULL)",lexicals.get_lexical_number(tmp));
#endif
                        }
                    }
                    else if (isnumber(*ptr) || *ptr == '"' || *ptr == '\'') { // 识别数值
                        value_result res;
                        off = parse_value(ptr,res);
                        string tmp = genstring(ptr,off);
                        if ((res.type == INT || res.type == HEX || res.type == OCT) && last_def_symbol_name != "") {
                            symbols.append_type(last_def_symbol_name,string("[") + tmp + string("]"));
                            upd_name_of_type = true;
                        }
                        int inserted_id = values.insert(string(output_value_type(res.type)),tmp);
#ifndef NO_SRC_MODE
                        printf("(%02d,%03d,%s)",lexicals.get_lexical_number("val"),inserted_id,tmp.c_str());
#else
                        printf("(%02d,%03d)",lexicals.get_lexical_number("val"),inserted_id);
#endif
#ifdef DEBUG
                        cout << genstring(ptr,off) << "\t" << string(output_value_type(res.type)) << "\n";
#endif
                    }
                    else {
                        errors.raise_error(line_number,ptr-linestart,"unknow character, skipped.");
#ifdef DEBUG
                        printf("error byte=%d\n",(int)*ptr);
#endif
                        off = 1;
                    }
                    assert(off != 0);
                    ptr += off;
                }
            }
            if (!upd_name_of_type) {
                name_of_type = "";
                last_def_symbol_name = "";
                symbol_ready_commit = make_pair(string(""),string(""));
            }
        }
        if (br_curly != 0) errors.raise_error(last_curly_line,last_curly_col,"Bracket didn't match when source code ends.");
    }
    void print_result() {
        errors.print_err();
        printf("\n");
        lexicals.print_all();
        printf("\n");
        symbols.print_all();
        printf("\n");
        values.print_all();
    }
    lexical_parser() {
        for (string x : type_qualifiers) {
            lexicals.insert(x);
        }
        for (string x : types) {
            lexicals.insert(x);
        }
        for (string x : keys) {
            lexicals.insert(x);
        }
        for (string x : builtin_functions) {
            lexicals.insert(x);
        }
        for (string x : ops) {
            lexicals.insert(x);
            trie.insertNode(x.c_str());
        }
        for (char x : control_ops) {
            string tmp = "0";
            tmp[0] = x;
            lexicals.insert(tmp);
            trie.insertNode(tmp.c_str());
        }
        for (pair<char,char> x : control_ops_pairwise) {
            string tmp = "0";
            tmp[0] = x.first;
            lexicals.insert(tmp);
            trie.insertNode(tmp.c_str());
            tmp[0] = x.second;
            lexicals.insert(tmp);
            trie.insertNode(tmp.c_str());
        }
        lexicals.post_init();
    }
private:
    opTrie trie;
    error_manager errors;
    value_manager values;
    lexical_manager lexicals;
    symbol_manager symbols;
    int read_keyword(const char *str) {//start with alpha or _ 
        int offset = 0;
        while (isdigit(*str) || isalpha(*str) || *str == '_') {
            str ++;
            offset ++;
        }
        return offset;
    }
    string genstring(const char *s,int off) {
        static char buffer[256];
        for (int i=0;i<off;i++) buffer[i] = s[i];
        buffer[off] = 0;
        return string(buffer);
    }
    string genptr_level(int level) {
        string res;
        for (int i=0;i<level;i++) res += "*";
        return res;
    }
};