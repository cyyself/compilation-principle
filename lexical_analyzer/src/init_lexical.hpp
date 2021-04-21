#include <set>
#include <string>
#include <utility>

std::set <std::string> type_qualifiers = std::set <std::string> ({
    "const","static","volatile",
    "register","inline","extern"
});

std::set <std::string> types = std::set <std::string>({
    "void","int","long","short",
    "float","double","char",
    "unsigned","signed","struct",
    "union"
});

std::set <std::string> keys = std::set <std::string> ({
    "if","else","goto","switch",
    "case","do","while","for",
    "continue","break","return",
    "default","sizeof","true","false"
});

std::set <std::string> ops = std::set <std::string> ({
    "+","-","*","/","<",">","==",
    "!=","<<",">>","++","--","&&",
    "||","&","|","^","!","~",">=",
    "<=","?",":","=","+=","-=","*=",
    "/=","<<=",">>=","&=","|=","^="
});

std::set <char> control_ops = std::set<char> ({
    ',',';'
});

std::set<std::pair<char,char> > control_ops_pairwise = 
    std::set<std::pair<char,char> >({
        {'{','}'},
        {'(',')'},
        {'[',']'}
    });