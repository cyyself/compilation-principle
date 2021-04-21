#include <cctype>
namespace value_parser {
    
    enum value_type {
        ERROR_TYPE,INT,FLOAT,STR,OCT,HEX,CHAR
    };

    struct value_result {
        value_type type;
        const char *value;
        int valuelen;
    };

    static const char *value_type_str[] = {
        "error_type",
        "int",
        "float",
        "string",
        "oct",
        "hex",
        "char"
    };

    const char* output_value_type(value_type t) {
        return value_type_str[t];
    }

    int parse_value(const char *str,value_result &ret) {
        ret.type = ERROR_TYPE;
        ret.value = str;
        int &offset = ret.valuelen = 0;
        if (str[0] == '\'') { // 检测到字符常量
            ret.type = CHAR;
            if (str[2] == '\'') {
                offset = 3;
            }
            else {
                if (str[1] == '\\' && str[3] == '\'') {
                    offset = 4;
                }
                else {
                    ret.type = ERROR_TYPE;
                }
            }
            return offset;
        }
        else if (str[0] == '"') { // 检测到字符串
            offset ++;
            bool last_zy = false;
            while (str[offset] != EOF && (last_zy || str[offset] != '"')) {
                last_zy = (str[offset] == '\\'); // 考虑转义符，例如\"不应该认为是字符串结束
                offset ++;
            }
            if (str[offset] == EOF) {
                ret.type = ERROR_TYPE;
            }
            else {
                offset ++;
                ret.type = STR;
                ret.valuelen = offset;
            }
            return offset;
        }
        else if (isdigit(str[0]) || str[0] == '-') {// 检测到数值
            ret.type = INT;
            bool neg = str[0] == '-';
            if (neg) offset ++;
            while (str[offset] != EOF) {
                if (str[offset] == '-') {
                    if (str[offset-1] != 'e') break;
                }
                else if (str[offset] == '.') {
                    ret.type = FLOAT;
                }
                else if (str[offset] == 'e') {
                    ret.type = FLOAT;
                }
                else if (str[offset] == '0' && offset == neg) {
                    ret.type = OCT;
                }
                else if (str[offset] == 'x') {
                    ret.type = HEX;
                }
                else if (!isdigit(str[offset])) {
                    // 如果不是16进制还出现了其它字符，那么就认为可能是到达了边界，直接停止
                    if (!(ret.type == HEX && 
                        (
                            (str[offset] >= 'a' && str[offset] <= 'f') || (str[offset] >= 'A' && str[offset] <= 'F'))
                        )
                    ) {
                        break;
                    }
                }
                offset ++;
            }
            if (ret.type == OCT) {
                if (offset == 1) ret.type = INT; // 考虑八进制数为0的情况，转为INT方便使用
                // 考虑八进制数如果出现了别的数字的情况
                for (int i=1;i<offset;i++) if (str[i] == '8' || str[i] == '9') {
                    ret.type = ERROR_TYPE;
                }
            }
            return offset;
        }
        else {
            return 1; // 不可能出现这种情况，但还是返回一个值来跳过这里
        }
    }
#ifdef DEBUG
    void runtest() {
        const char *s = "123 1.2e-10 1.1 0123 0x3f3f 0.1 \"asdf\\\"\"  123 0987";
        value_result res;
        const char *ptr = s;
        while (*ptr) {
            while (!isdigit(*ptr) && !(*ptr == '-') && !(*ptr == '"')) ptr ++;
            ptr += parse_value(ptr,res);
            printf("%s\t",output_value_type(res.type));
            for (int i=0;i<res.valuelen;i++) printf("%c",res.value[i]);
            printf("\n");
        }
    }
#endif
}