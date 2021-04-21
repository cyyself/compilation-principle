lexical_analyzer: src/main.cpp
	g++ -std=c++17 src/main.cpp -o lexical_analyzer -fsanitize=address

.PHONY: test1 test2 clean
test1: lexical_analyzer testcase1.c
	@echo "Testing Testcase1.c"
	@./lexical_analyzer < testcase1.c

test2: lexical_analyzer testcase2.c
	@echo "Testing Testcase2.c"
	@./lexical_analyzer < testcase2.c

test: lexical_analyzer testcase1.c testcase2.c
	@echo "Testing Testcase1.c"
	@./lexical_analyzer < testcase1.c
	@echo "Testing Testcase2.c"
	@./lexical_analyzer < testcase2.c

clean:
	rm lexical_analyzer