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
        const char *ptr = s;
        bool comment_single = false, comment_multiline = false; // 注释识别状态机
        int line_number = 0; // 当前的行号
        const char* linestart = ptr;
        bool lastistype = false;
        string name_of_type; // 当前的类型，用于进行类型的识别
        int br_round = 0, br_curly = 0, br_square = 0; // 处理括号匹配
        pair <string,string> symbol_ready_commit; // 用于识别结构体和联合体定义，需要等待识别到对应的定义字符才提交更改
        while (*ptr) {
            bool upd_name_of_type = false; // 处理上次类型是否更新，如果未更新则在最后清空name of type
            if (*ptr == '\n') { // 处理换行
                line_number ++;
                linestart = ptr;
                if (comment_single) { // 处理多行注释
                    comment_single = false;
                }
                ptr ++;
            }
            else { // 其它字符
                if (comment_multiline || comment_single) { // 位于注释状态中
                    if (*ptr == '*' && *(ptr+1) == '/') comment_multiline = false; // 处理多行注释结束
                    ptr ++;
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
                        upd_name_of_type = true;
                        off = 1;
                    }
                    else if (*ptr == ';') { // 语句完毕，考虑括号适配
                        off = 1;
                        symbol_ready_commit = make_pair(string(""),string(""));
                        if (br_round != 0 || br_square != 0) {
                            errors.raise_error(line_number,ptr-linestart,"Bracket didn't match when sentense end.");
                            br_round = 0;
                            br_square = 0;
                        }
                        //TODO: 输出
                    }
                    // 以下是处理括号匹配
                    else if (*ptr == '(' || *ptr == ')') {
                        if (*ptr == '(') {
                            br_round ++;
                        }
                        else {
                            br_round --;
                            if (br_round < 0) errors.raise_error(line_number,ptr-linestart,"Bracket didn't match.");
                        }
                        off = 1;
                    }
                    else if (*ptr == '{' || *ptr == '}') {
                        if (*ptr == '{') {
                            br_curly ++;
                            if (symbol_ready_commit != make_pair(string(""),string(""))) {
                                symbols.insert(symbol_ready_commit.first,symbol_ready_commit.second);
                                symbol_ready_commit = make_pair(string(""),string(""));
                            }
                        }
                        else {
                            br_curly --;
                            if (br_curly < 0) errors.raise_error(line_number,ptr-linestart,"Bracket didn't match.");
                        }
                        off = 1;
                    }
                    else if (*ptr == '[' || *ptr == ']') {
                        if (*ptr == '[') {
                            br_square ++;
                        }
                        else {
                            br_square --;
                            if (br_square < 0) errors.raise_error(line_number,ptr-linestart,"Bracket didn't match.");
                        }
                        off = 1;
                    }
                    else if (isalpha(*ptr) || *ptr == '_') { // 识别标识符
                        off = read_keyword(ptr);
                        // TODO: 进行识别
                        string tmp = genstring(ptr,off);
                        if (types.find(tmp) != types.end()) { // 识别为类型
                            name_of_type = tmp;
                            upd_name_of_type = true;
                        }
                        else {
                            if (name_of_type != "") { // 识别为正在定义符号（注意特殊处理struct和union）
                                if (name_of_type == "struct" || name_of_type == "union") { // 类型更新为自定义类型
                                    symbol_ready_commit = make_pair(name_of_type,tmp);
                                    // symbols.insert(name_of_type,name_of_type + string(" ") +tmp);
                                    name_of_type = tmp;
                                    upd_name_of_type = true;
                                    // TODO: 输出
                                }
                                else {
                                    if (types.find(name_of_type) != types.end() || symbols.has_struct_or_union(name_of_type)) {
                                        symbols.insert(name_of_type,tmp);
                                    }
                                    else {
                                        errors.raise_error(line_number,ptr-linestart,"Undifined type");
                                    }
                                    // TODO: 识别为类型输出
                                }
                            }
                            else {
                                // TODO: 直接翻译为对应词法
                            }
                        }
#ifdef DEBUG
                        cout << tmp << "\n";
#endif
                    }
                    else if (trie.firstExist(*ptr)) { // 识别符号
                        off = trie.query(ptr);
                        string tmp = genstring(ptr,off);
                        if (tmp == "*" || tmp == ",") upd_name_of_type = true; // 如果定义的是函数指针或者逗号，需要保留之前所定义的类型。
                        if (off == 0) {
                            // 无法识别为任何符号的模式
                            errors.raise_error(line_number,ptr-linestart,"Operator Decode Error");
                            off = 1;
                        }
                        else {
#ifdef DEBUG
                            cout << tmp << "\n";
#endif
                            // TODO: 进行识别
                        }
                    }
                    else if (isnumber(*ptr) || *ptr == '"' || *ptr == '\'') { // 识别数值
                        value_result res;
                        off = parse_value(ptr,res);
                        values.insert(string(output_value_type(res.type)),genstring(ptr,off));
#ifdef DEBUG
                        cout << genstring(ptr,off) << "\t" << string(output_value_type(res.type)) << "\n";
#endif
                        // TODO: 加入输出buffer
                    }
                    else {
                        errors.raise_error(line_number,ptr-linestart,"Unknow Character");
                        off = 1;
                    }
                    assert(off != 0);
                    ptr += off;
                }
            }
            if (!upd_name_of_type) {
                name_of_type = "";
                symbol_ready_commit = make_pair(string(""),string(""));
            }
        }
    }
    void print_result() {
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
};