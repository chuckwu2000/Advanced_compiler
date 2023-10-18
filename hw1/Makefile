LLVM_CONFIG=/home/chuck/NYCU_HW/Advanced_compiler/llvm_build/bin/llvm-config

CXX=`$(LLVM_CONFIG) --bindir`/clang
CXXFLAGS=`$(LLVM_CONFIG) --cppflags` -fPIC -fno-rtti
LDFLAGS=`$(LLVM_CONFIG) --ldflags`
IRFLAGS=-Xclang -disable-O0-optnone -fno-discard-value-names -S -emit-llvm
OPT=`$(LLVM_CONFIG) --bindir`/opt

.PHONY: all test1 test2 run clean
all: hw1.so test

test1: test1.ll

test2: test2.ll

hw1.so: hw1.cpp
	$(CXX) -shared -o $@ $< $(CXXFLAGS) $(LDFLAGS)

test1.ll: test1.c
	$(CXX) $(IRFLAGS) -o $@ $<

test2.ll: test2.c
	$(CXX) $(IRFLAGS) -o $@ $<

run: test1.ll hw1.so
	#$(OPT) -time-passes -disable-output -load-pass-plugin=./hw1.so -passes='hw1' $<
	$(OPT) -disable-output -load-pass-plugin=./hw1.so -passes='hw1' $<

clean:
	rm -f *.o *.ll *.so
