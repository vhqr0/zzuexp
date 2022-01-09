libsha.so: sha.cpp
	g++ -fPIC -shared sha.cpp -O2 -o libsha.so

test.out: libsha.so sha_test.cpp
	g++ sha_test.cpp -L. -lsha -o test.out

test: test.out
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./test.out
