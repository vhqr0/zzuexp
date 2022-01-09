libsecp256k1.so: secp256k1.cpp
	g++ -fPIC -shared secp256k1.cpp -lgmp -O2 -o libsecp256k1.so

test.out: libsecp256k1.so secp256k1_test.cpp
	g++ secp256k1_test.cpp -L. -lgmp -lsecp256k1 -o test.out

test: test.out
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./test.out
