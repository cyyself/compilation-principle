#include <utility>
#include <vector>
#include <string>
#include <iostream>

using std::cerr;
using std::vector;
using std::string;
using std::tuple;

class error_manager {
public:
    void raise_error(int line,int col,string msg) {
        errors.push_back(tie(line,col,msg));
#ifdef DEBUG
        cerr << "\nerror: line " << line << ", col " << col << ", " << msg << "\n";
        if (line-1 < lines.size()) {
            cerr << lines[line-1] << "\n";
            for (int i=1;i<col;i++) cerr << " ";
            cerr << "^\n";
        }
#endif
    }
    void print_err() {
        for (auto cur : errors) {
            int line, col;
            string msg;
            tie(line,col,msg) = cur;
            cerr << "\n\033[0;31merror\033[0m: line " << line << ", col " << col << ", " << msg << "\n";
            if (line-1 < lines.size()) {
                for (auto c:lines[line-1]) {
                    if (c == '\t') cerr << "    "; // 对制表符特殊处理，美化输出结果
                    else cerr << c;
                }
                cerr << "\n";
                for (int i=0;i<col;i++) {
                    if (lines[line-1][i] == '\t') {
                        cerr << "   ";
                    }
                    if (i != col-1) cerr << " ";
                }
                cerr << "^\n";
            }
        }
    }
    void init_lines(const char *ptr) {
        const char *ptr_start = ptr;
        string buf;
        while (*ptr && *ptr != EOF) {
            if (*ptr != '\r') {
                if (*ptr == '\n') {
                    lines.push_back(buf);
                    buf = "";
                }
                else {
                    buf += *ptr;
                }
            }
            ptr ++;
        }
        if (buf != "") lines.push_back(buf);
        filesize = ptr - ptr_start;
    }
    int getlines() {
        return lines.size();
    }
    int getfilesize() {
        return filesize;
    }
private:
    vector<tuple<int,int,string> > errors;
    vector<string> lines;
    int filesize = 0;
};