librsa.so: rsa.cpp
	g++ -fPIC -shared rsa.cpp -lgmp -O2 -o librsa.so

test.out: librsa.so rsa_test.cpp
	g++ rsa_test.cpp -L. -lgmp -lrsa -o test.out

test: test.out
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./test.out
