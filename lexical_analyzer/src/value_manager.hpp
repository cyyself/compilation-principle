#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <utility>

using std::string;
using std::map;
using std::vector;
using std::pair;

class value_manager { // 用于管理所有出现的值
public:
    int insert(string type, string value) {
        if (value_id.find(value) != value_id.end()) return value_id[value];
        else {
            value_id[value] = all_value.size();
            all_value.push_back(make_pair(type,value));
            return value_id[value];
        }
    }
    void print_all() {
        printf("----- Const Table BEGIN -----\n");
        printf("|No.|  Type  | Value \n");
        printf("-----------------------------\n");
        for (int i=0;i<all_value.size();i++) {
            printf("|%03d|%*s|%s\n",i,8,all_value[i].first.c_str(),all_value[i].second.c_str());
        }
        printf("----- Const Table  END  -----\n");
    }
private:
    vector <pair<string,string> > all_value;
    map <string,int> value_id; // 对于value，value一定能对应到type以及id，因此这里只存储value
};