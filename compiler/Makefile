compiler: src/*
	g++ -std=c++17 src/main.cpp -o compiler -fsanitize=address

.PHONY: test1 test2 test3 test clean
test1: compiler testcase1.c plot.py
	@echo "Testing Testcase1.c"
	@./compiler < testcase1.c > tmp.txt
	@cat tmp.txt
	@python3 plot.py < tmp.txt


test2: compiler testcase2.c plot.py
	@echo "Testing Testcase2.c"
	@./compiler < testcase2.c > tmp.txt
	@cat tmp.txt
	@python3 plot.py < tmp.txt

test3: compiler testcase3.c plot.py
	@echo "Testing Testcase3.c"
	@./compiler < testcase3.c > tmp.txt
	@cat tmp.txt
	@python3 plot.py < tmp.txt

test4: compiler testcase4.c plot.py
	@echo "Testing Testcase4.c"
	@./compiler < testcase4.c > tmp.txt
	@cat tmp.txt
	@python3 plot.py < tmp.txt

test5: compiler testcase5.c plot.py
	@echo "Testing Testcase5.c"
	@./compiler < testcase5.c > tmp.txt

clean:
	rm compiler