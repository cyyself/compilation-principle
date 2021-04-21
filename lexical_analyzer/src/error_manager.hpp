#include <utility>
#include <vector>
#include <string>

#ifdef DEBUG
    #include <iostream>
    using std::cerr;
#endif

using std::vector;
using std::string;
using std::tuple;

class error_manager {
public:
    void raise_error(int line,int col,string msg) {
        errors.push_back(tie(line,col,msg));
#ifdef DEBUG
        cerr << "error: line " << line << ", col " << col << ", " << msg << "\n";
#endif
    }
private:
    vector<tuple<int,int,string> > errors;
};