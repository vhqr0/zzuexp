libaes.so: aes.cpp
	g++ -fPIC -shared aes.cpp -O2 -o libaes.so

test.out: libaes.so aes_test.cpp
	g++ aes_test.cpp -L. -laes -o test.out

test: test.out
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./test.out
