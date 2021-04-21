class opTrie {
public:
    struct Node {
        Node *ch[128];
        bool exist;
        Node() {
            exist = false;
            for (int i=0;i<128;i++) ch[i] = NULL;
        }
    };
    opTrie() {
        root = new Node;
    }
    ~opTrie() {
        deleteNode(root);
    }
    int query(const char *s) {
        int maxlen = 0;
        Node *cur = root;
        int curlen = 0;
        while (*s) {
            if (*s & 128) break;
            if (cur->ch[*s]) {
                cur = cur->ch[*s];
                curlen ++;
                if (cur->exist) maxlen = curlen;
            }
            else {
                break;
            }
            s ++;
        }
        return maxlen;
    }
    bool firstExist(char c) {
        if (c & 128) return false;
        return root->ch[c] != NULL;
    }
    bool insertNode(const char *s) {
        Node *cur = root;
        while (*s) {
            if (*s & 128) return false;
            if (cur->ch[*s] == NULL) {
                cur->ch[*s] = new Node;
            }
            cur = cur->ch[*s];
            s ++;
        }
        if (!cur->exist) {
            cur->exist = true;
            return true;
        }
        else return false;
    }
private:
    Node *root;
    void deleteNode(Node *node) {
        for (int i=0;i<128;i++) if (node->ch[i]) deleteNode(node->ch[i]);
        delete node;
    }
};