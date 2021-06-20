#include "grammar_parser.hpp"

struct four_tuple_node {
    string op,arg1,arg2,result;
    four_tuple_node(string _op,string _arg1="",string _arg2="",string _result="") {
        op = _op;
        arg1 = _arg1;
        arg2 = _arg2;
        result = _result;
    }
    string to_mips() {
        if (op == "label") return result + string(":");
        else {
            if (op == "li") return string("li ") + result + string(",") + arg1;
            else if (op == "sw") return string("sw ") + result + string(",") + arg1;
            else if (op == "lw") return string("lw ") + result + string(",") + arg1;
            else if (op == "j") return string("j ") + result;
            else if (op == "move") return string("move ") + result + string(",") + arg1;
            else if (op == "syscall") return string("syscall");
            else if (op == "jal") return string("jal ") + result;
            else if (op == "jr") return string("jr ") + result;
            else if (op == "bnez") return string("bnez ") + arg1 + string(",") + result;
            else return op + string(" ") + result + string(",") + arg1 + string(",") + arg2; // all alu instr
        }
    }
    string to_four_tuple(map<string,string> &gp_map) {
        string _op = (gp_map.find(op) == gp_map.end()) ? op : gp_map[op];
        string _arg1 = (gp_map.find(arg1) == gp_map.end()) ? arg1 : gp_map[arg1];
        string _arg2 = (gp_map.find(arg2) == gp_map.end()) ? arg2 : gp_map[arg2];
        string _res = (gp_map.find(result) == gp_map.end()) ? result : gp_map[result];
        return string("(") + _op + string(",") + _arg1 + string(",") + _arg2 + string(",") + _res + string(")");
    }
};

const char* myitoa(int i) {
    static char buffer[20];
    sprintf(buffer,"%d",i);
    return buffer;
}

class compiler{
public:
    compiler(const char *buffer) {
        parser = new grammar_parser(buffer);
        symbols = &(parser->lex.symbols);
        values = &(parser->lex.values);
        lex = &(parser->lex.lexicals);
    }
    void compile() {
        TreeNode* treeRT = parser->parse();
        // 首先使用li和sw将常量载入内存
        for (auto x : values->all_value) {
            if (x.first == "int" || x.first == "oct" || x.first == "hex") {
                middle_code.emplace_back("li",x.second,"","$t0");
                middle_code.emplace_back(
                    "sw",
                    string(myitoa(4 * const_cnt)) + string("($gp)"),
                    "",
                    "$t0"
                );
                const_size ++;
            }
            const_cnt ++;
        }
        middle_code.emplace_back("j","","","main");
        exp_translate(treeRT);
        scan();
    }
    void output_four_tuple() {
        cout << "\n----- Four Tuple CODE BEGIN -----\n";
        for (int i=const_size*2;i<middle_code.size();i++) {
            if (middle_code[i].op == "li" && i + 1 < middle_code.size()) {
                if (middle_code[i+1].op == "sw" 
                    && middle_code[i].arg1 == middle_code[i+1].arg1
                    && middle_code[i].result == middle_code[i+1].result) continue;
            }
            cout << i-const_size*2 << ":" << middle_code[i].to_four_tuple(gp_map) << "\n";
        }
        cout << "----- Four Tuple CODE  END  -----\n";
    }
    void output_mips() {
        cout << "\n----- MIPS CODE BEGIN -----\n";
        for (auto x : middle_code) {
            cout << x.to_mips() << "\n";
        }
        cout << "----- MIPS CODE  END  -----\n";
    }
    // 用于对表达式进行计算，t_reg是结果所放置的t寄存器编号，并保证不会去修改更小的寄存器的值
    void exp_translate(TreeNode *tr,int t_reg = 0) { 
        if (tr->type == OP) {
            string op = lex->get_lexical_str(tr->token.first);
            if (op == "=") {
                if (tr->child[0]->type == SINGLEVAR) {
                    exp_translate(tr->child[1],t_reg);
                    middle_code.emplace_back(
                        "sw",
                        string(myitoa(4 * (const_cnt+tr->child[0]->token.second))) + string("($gp)"),
                        "",
                        string("$t") + string(myitoa(t_reg))
                    );
                }
                else assert(false);
            }
            else if (op == "+") {
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "addu",
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == "-") {
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "subu",
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == "*") {
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "mulu",
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == "/") {
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "divu",
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == "==") { // 通过mipsel-linux-gnu-gcc -S的输出结果发现GCC是这么实现的
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "xor",
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg))
                );
                middle_code.emplace_back(
                    "sltiu",
                    string("$t") + string(myitoa(t_reg)),
                    string("1"),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == "!=") {
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "xor",
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg))
                );
                middle_code.emplace_back(
                    "xori",
                    string("$t") + string(myitoa(t_reg)),
                    string("1"),
                    string("$t") + string(myitoa(t_reg))
                );
                middle_code.emplace_back(
                    "sltiu",
                    string("$t") + string(myitoa(t_reg)),
                    string("1"),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == "<") {
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "slt",
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == ">") {
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "slt",
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == "<=") {
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "sle",
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == ">=") {
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "sle",
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == "&&") { // 由于之前已经检查过运算的类型，因此可以直接使用Bitwise 运算符替代
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "and",
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == "&&" || op == "&") { // 由于之前已经检查过运算的类型，因此可以直接使用Bitwise 运算符替代
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "and",
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == "||" || op == "|") {
                exp_translate(tr->child[0],t_reg+1);
                exp_translate(tr->child[1],t_reg+2);
                middle_code.emplace_back(
                    "or",
                    string("$t") + string(myitoa(t_reg+1)),
                    string("$t") + string(myitoa(t_reg+2)),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else if (op == "!") {
                exp_translate(tr->child[1],t_reg);
                middle_code.emplace_back(
                    "xori",
                    string("$t") + string(myitoa(t_reg)),
                    string("1"),
                    string("$t") + string(myitoa(t_reg))
                );
            }
            else assert(false);
        }
        else if (tr->type == FUNCTION_CALL) {
            string function_name = symbols->get_symbol_str(tr->token.second);
            if (function_name == "get" || function_name == "put") {
                for (auto ch : tr->child) {
                    if (function_name == "get") {
                        middle_code.emplace_back(
                            "li",
                            "5",
                            "",
                            "$v0"
                        );
                        middle_code.emplace_back("syscall");
                        if (ch->type == SINGLEVAR) { // 存储得到的值
                            middle_code.emplace_back(
                                "sw",
                                string(myitoa(4 * (const_cnt + ch->token.second))) + string("($gp)"),
                                "",
                                string("$v0")
                            );
                        }
                        else assert(false);
                    }
                    else { // put
                        exp_translate(ch,t_reg);
                        middle_code.emplace_back(
                            "move",
                            string("$t") + string(myitoa(t_reg)),
                            "",
                            "$a0"
                        );
                        middle_code.emplace_back(
                            "li",
                            "1",
                            "",
                            "$v0"
                        );
                        middle_code.emplace_back("syscall");
                    }
                }
            }
            else { // 其它自定义函数
                for (int i=0;i<tr->child.size();i++) { // 首先将参数放入a寄存器
                    exp_translate(tr->child[i],t_reg);
                    middle_code.emplace_back(
                        "move",
                        string("$t") + string(myitoa(t_reg)),
                        "",
                        "$a" + string(myitoa(i))
                    );
                }
                middle_code.emplace_back(
                    "jal",
                    "",
                    "",
                    function_name
                );
            }
        }
        else if (tr->type == SINGLEVAR) {
            middle_code.emplace_back(
                "lw",
                string(myitoa(4 * (const_cnt + tr->token.second))) + string("($gp)"),
                "",
                string("$t") + string(myitoa(t_reg))
            );
        }
        else if (tr->type == VAL) {
            middle_code.emplace_back(
                "lw",
                string(myitoa(4 * tr->token.second)) + string("($gp)"),
                "",
                string("$t") + string(myitoa(t_reg))
            );
        }
        else if (tr->type == BLOCK) {
            // 代码块
            for (auto ch : tr->child) exp_translate(ch,t_reg);
        }
        else if (tr->type == VARDECLEAERS) { // 变量声明
            for (auto ch : tr->child) exp_translate(ch,t_reg);
        }
        else if (tr->type == SINGLETYPEVAR) {
            if (tr->child.size() == 4) { // 赋初始值
                middle_code.emplace_back(
                    "lw",
                    string(myitoa(4 * tr->child[3]->token.second)) + string("($gp)"),
                    "",
                    string("$t") + string(myitoa(t_reg))
                );
                middle_code.emplace_back(
                    "sw",
                    string(myitoa(4 * (const_cnt + tr->child[2]->token.second))) + string("($gp)"),
                    "",
                    string("$t") + string(myitoa(t_reg))
                );
            }
        }
        else if (tr->type == FUNCTION_DECLARE) {
            string function_name = symbols->get_symbol_str(tr->token.second);
            middle_code.emplace_back("label","","",function_name);
            for (int i=0;i<tr->child[2]->child.size();i++) {
                TreeNode *param = tr->child[2]->child[i];
                middle_code.emplace_back(
                    "sw",
                    string(myitoa(4 * (const_cnt + param->child[2]->token.second))) + string("($gp)"),
                    "",
                    "$a" + string(myitoa(i))
                );
            }
            exp_translate(tr->child[3],t_reg);
            if (function_name == "main") {
                middle_code.emplace_back("li","10","","$v0");
                middle_code.emplace_back("syscall");
            }
            else {
                middle_code.emplace_back(
                    "jr",
                    "",
                    "",
                    "$ra"
                );
            }
        }
        else if (tr->type == RETURN) {
            exp_translate(tr->child[0],t_reg);
            middle_code.emplace_back(
                "move",
                string("$t") + string(myitoa(t_reg)),
                "",
                "$v0"
            );
        }
        else if (tr->type == IF) {
            bool has_else = (tr->child.size() == 3);
            exp_translate(tr->child[0],t_reg);
            middle_code.emplace_back(
                "bnez",
                string("$t") + string(myitoa(t_reg)),
                "",
                "if_" + string(myitoa(if_count)) + "_true"
            );
            middle_code.emplace_back("j","","","if_" + string(myitoa(if_count)) + "_else");
            middle_code.emplace_back("label","","","if_" + string(myitoa(if_count)) + "_true");
            exp_translate(tr->child[1],t_reg);
            if (has_else) {
                middle_code.emplace_back("j","","","if_" + string(myitoa(if_count)) + "_end");
                middle_code.emplace_back("label","","","if_" + string(myitoa(if_count)) + "_else");
                exp_translate(tr->child[2],t_reg);
                middle_code.emplace_back("label","","","if_" + string(myitoa(if_count)) + "_end");
            }
            else {
                middle_code.emplace_back("label","","","if_" + string(myitoa(if_count)) + "_else");
            }
            if_count ++;
        }
        else if (tr->type == WHILE) {
            middle_code.emplace_back("label","","","while_" + string(myitoa(while_count)));
            exp_translate(tr->child[0],t_reg);
            middle_code.emplace_back(
                "bnez",
                string("$t") + string(myitoa(t_reg)),
                "",
                "while_" + string(myitoa(while_count)) + "_true"
            );
            middle_code.emplace_back("j","","","while_" + string(myitoa(while_count)) + string("_end"));
            middle_code.emplace_back("label","","","while_" + string(myitoa(while_count)) + string("_true"));
            exp_translate(tr->child[1],t_reg);
            middle_code.emplace_back("j","","","while_" + string(myitoa(while_count)));
            middle_code.emplace_back("label","","","while_" + string(myitoa(while_count)) + string("_end"));
            while_count ++;
        }
        else assert(false);
    }
    void scan() {
        for (int i=0;i<values->all_value.size();i++) {
            gp_map[string(myitoa(4 * i)) + string("($gp)")] = values->all_value[i].second;
        }
        for (int i=0;i<symbols->all_symbol.size();i++) {
            gp_map[string(myitoa(4 * (i + const_cnt))) + string("($gp)")] = symbols->all_symbol[i].second;
        }
        for (int i=0;i<middle_code.size();i++) {
            if (middle_code[i].op == "label") {
                gp_map[middle_code[i].result] = string("(") + string(myitoa(i-const_size*2)) + string(")");
            }
        }
        gp_map["li"] = "load";
        gp_map["lw"] = "load";
        gp_map["sw"] = "store";
        gp_map["jal"] = "call";
        gp_map["jr"] = "return";
        gp_map["bnez"] = "j!=0";
        gp_map["addu"] = "+";
        gp_map["subu"] = "-";
        gp_map["mulu"] = "*";
        gp_map["divu"] = "/";
        gp_map["sltiu"] = "<";
        gp_map["slt"] = "<";
        gp_map["sle"] = "<=";
        gp_map["and"] = "&";
        gp_map["or"] = "|";
        gp_map["xori"] = "xor";
    }
private:
    vector <four_tuple_node> middle_code;
    grammar_parser *parser;
    symbol_manager *symbols;
    value_manager *values;
    lexical_manager *lex;
    int if_count = 0;
    int while_count = 0;
    // 存储空间规划方案：在offset($gp)依次放入所有的常量与变量
    int const_cnt = 0;
    int const_size = 0;
    map <string,string> gp_map;
};