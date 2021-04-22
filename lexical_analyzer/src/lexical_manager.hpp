#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

using std::string;
using std::map;
using std::vector;
using std::min;

class lexical_manager { // 用于储存所有的词法
public:
    int insert(string s) {
        if (lexical_id.find(s) != lexical_id.end()) return -1;
        else {
            lexical_id[s] = all_lexical.size();
            all_lexical.push_back(s);
            return lexical_id[s];
        }
    }
    void post_init() {
        insert("sym");
        insert("val");
    }
    void print_all() {
        printf("----- Lexical Table BEGIN -----\n");
        for (int i=0;i<all_lexical.size();i+=4) {
            for (int j=i;j<min(i+4,(int)all_lexical.size());j++)
                printf("%02d %*s|",j,8,all_lexical[j].c_str());
            printf("\n");
        }
        printf("----- Lexical Table  END  -----\n");
    }
    bool lexical_exist(string s) {
        return lexical_id.find(s) != lexical_id.end();
    }
    int get_lexical_number(string s) {
        if (lexical_exist(s)) return lexical_id[s];
        else return -1;
    }
private:
    vector <string> all_lexical;
    map <string,int> lexical_id;
};