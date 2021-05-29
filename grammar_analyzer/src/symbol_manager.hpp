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
        printf("|No.|     Type     |field|Symbol \n");
        printf("------------------------------\n");
        for (int i=0;i<all_symbol.size();i++) {
            printf("|%03d|%*s|%*d|%s\n",i,14,all_symbol[i].first.c_str(),5,get_symbol_struct(i),all_symbol[i].second.c_str());
        }
        printf("----- Symbol Table  END  -----\n");
    }
    bool has_struct_or_union(string s) {
        if (has_defined(s)) {
            return (all_symbol[symbol_id[s]].first == "struct" || all_symbol[symbol_id[s]].first == "union");
        }
        else return false;
    }
    bool has_defined(string s) {
        return symbol_id.find(s) != symbol_id.end();
    }
    int get_symbol_id(string s) {
        if (has_defined(s)) {
            return symbol_id[s];
        }
        else return -1;
    }
    bool append_type(string s,string typetoappend) { // 用于给已有symbol的类型添加属性
        if (!has_defined(s)) return false;
        else {
            all_symbol[symbol_id[s]].first += typetoappend;
            return true;
        }
    }
    bool has_suffix(int id, string suffix) {
        if (id >= 0 && id < all_symbol.size()) {
            string tmp = all_symbol[id].first;
            if (tmp.length() >= suffix.length()) {
                if (tmp.substr(tmp.length()-suffix.length(),suffix.length()) == suffix) return true;
            }
        }
        return false;
    }
    bool has_prefix(int id, string prefix) {
        if (id >= 0 && id < all_symbol.size()) {
            string tmp = all_symbol[id].first;
            if (tmp.length() >= prefix.length()) {
                return tmp.substr(0,prefix.length()) == prefix;
            }
        }
        return false;
    }
    string get_symbol_str(int id) {
        if (id >= 0 && id < all_symbol.size()) return all_symbol[id].second;
        return "";
    }
    void add_to_struct(int id,int struct_id) {
        belong_to_struct[id] = struct_id;
    }
    int get_symbol_struct(int id) {
        if (belong_to_struct.find(id) != belong_to_struct.end()) {
            return belong_to_struct[id];
        }
        else return -1;
    }
private:
    vector <pair <string,string> > all_symbol;//存储类型和名称，方便后续进行判断
    map <int,int> belong_to_struct; // 这个符号所属的struct
    map <string,int> symbol_id;//symbol_id只存储名称，便于反查类型
};