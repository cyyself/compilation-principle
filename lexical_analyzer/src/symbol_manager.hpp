#include <string>
#include <map>
#include <vector>
#include <utility>

using std::pair;
using std::string;
using std::map;
using std::vector;

class symbol_manager { // 用于管理出现的所有符号，最后打印输出符号表
public:
    int insert(string type, string name) {
        if (symbol_id.find(name) != symbol_id.end()) return -1;
        else {
            symbol_id[name] = all_symbol.size();
            all_symbol.push_back(make_pair(type,name));
            return symbol_id[name];
        }
    }
    void print_all() {
        printf("----- Symbol Table BEGIN -----\n");
        printf("|No.|  Type  | Symbol |\n");
        printf("-----------------------\n");
        for (int i=0;i<all_symbol.size();i++) {
            printf("|%03d|%*s|%*s|\n",i,8,all_symbol[i].first.c_str(),8,all_symbol[i].second.c_str());
        }
        printf("----- Symbol Table  END  -----\n");
    }
    bool has_struct_or_union(string s) {
        if (symbol_id.find(s) != symbol_id.end()) {
            return (all_symbol[symbol_id[s]].first == "struct" || all_symbol[symbol_id[s]].first == "union");
        }
        else return false;
    }
private:
    vector <pair <string,string> > all_symbol;//存储类型和名称，方便后续进行判断
    map <string,int> symbol_id;//symbol_id只存储名称，便于反查类型
};