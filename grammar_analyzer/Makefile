grammar_analyzer: src/*
	g++ -std=c++17 src/main.cpp -o grammar_analyzer -fsanitize=address

.PHONY: test1 test2 test3 test clean
test1: grammar_analyzer testcase1.c
	@echo "Testing Testcase1.c"
	@./grammar_analyzer < testcase1.c

test2: grammar_analyzer testcase2.c
	@echo "Testing Testcase2.c"
	@./grammar_analyzer < testcase2.c

test3: grammar_analyzer testcase3.c
	@echo "Testing Testcase3.c"
	@./grammar_analyzer < testcase3.c

test4: grammar_analyzer testcase4.c
	@echo "Testing Testcase4.c"
	@./grammar_analyzer < testcase4.c

test: grammar_analyzer testcase1.c testcase2.c testcase3.c testcase4.c
	@echo "Testing Testcase1.c"
	@./grammar_analyzer < testcase1.c
	@echo "Testing Testcase2.c"
	@./grammar_analyzer < testcase2.c
	@echo "Testing Testcase3.c"
	@./grammar_analyzer < testcase3.c
	@echo "Testing Testcase4.c"
	@./grammar_analyzer < testcase4.c

clean:
	rm grammar_analyzer