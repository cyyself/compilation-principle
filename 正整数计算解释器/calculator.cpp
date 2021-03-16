#include <bits/stdc++.h>
using namespace std;
string buf;
int main() {
	while (getline(cin,buf)) {
		buf += '\n';
		stack <int> s;
		s.push('+');
		string tmp;
		bool last_stat = true;//0: operator, 1:number
		for (char c : buf) {
			if (c == ' ') continue;
			bool cur_stat = (c >= '0' && c <= '9');
			if (cur_stat != last_stat || c == '\n') {
				if (tmp == "") continue;
				if (s.size() & 1) {
					if (s.top() == '+' || s.top() == '-') s.push(atoi(tmp.c_str()));
					else if (s.top() == '*' || s.top() == '/') {
						int op = s.top();
						s.pop();
						int l_num = s.top();
						int r_num = atoi(tmp.c_str());
						s.pop();
						s.push(op == '*' ? l_num * r_num : l_num / r_num);
					}
				}
				else s.push(tmp[0]);
				tmp = "";
				tmp += c;
			}
			else tmp += c;
			last_stat = cur_stat;
		}
		int result = 0;
		while (!s.empty()) {
			int num = s.top();
			s.pop();
			int op = s.top();
			s.pop();
			if (op == '+') result += num;
			else result -= num;
		}
		printf("%d\n",result);
	}
	return 0;
}
