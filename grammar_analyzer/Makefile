lexical_analyzer: src/*
	g++ -std=c++17 src/main.cpp -o lexical_analyzer -fsanitize=address

.PHONY: test1 test2 test3 test clean
test1: lexical_analyzer testcase1.c
	@echo "Testing Testcase1.c"
	@./lexical_analyzer < testcase1.c

test2: lexical_analyzer testcase2.c
	@echo "Testing Testcase2.c"
	@./lexical_analyzer < testcase2.c

test3: lexical_analyzer testcase3.c
	@echo "Testing Testcase3.c"
	@./lexical_analyzer < testcase3.c

test: lexical_analyzer testcase1.c testcase2.c testcase3.c
	@echo "Testing Testcase1.c"
	@./lexical_analyzer < testcase1.c
	@echo "Testing Testcase2.c"
	@./lexical_analyzer < testcase2.c
	@echo "Testing Testcase3.c"
	@./lexical_analyzer < testcase3.c

clean:
	rm lexical_analyzer