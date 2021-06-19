#include "grammar_parser.hpp"

struct four_tuple_node {
    string op,arg1,arg2,result;
    four_tuple_node(string _op,string _arg1,string _arg2,string _result) {
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
        }
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
        parser->parse();
        symbols = &(parser->lex.symbols);
        values = &(parser->lex.values);
    }
    void compile() {
        // 首先使用li和sw将常量载入内存
        int const_cnt = 0;
        for (auto x : values->all_value) {
            if (x.first == "int" || x.first == "oct" || x.first == "hex") {
                target_code.emplace_back("li",x.second,"","t0");
                target_code.emplace_back("sw",string("(") + string(myitoa(4 * const_cnt)) + string(")gp"),"","t0");
            }
            const_cnt ++;
        }
        target_code.emplace_back("j","","","main");

    }
    void output_four_tuple() {
        int id = 1;
        for (auto x : target_code) {
            cout << id << ":" << "(" << x.op << "," << x.arg1 << "," << x.arg2 << "," << x.result << ")\n";
            id ++;
        }
    }
    void output_mips() {
        for (auto x : target_code) {
            cout << x.to_mips() << "\n";
        }
    }
private:
    vector <four_tuple_node> target_code;
    grammar_parser *parser;
    symbol_manager *symbols;
    value_manager *values;
    int if_count = 0;
    int while_count = 0;
};