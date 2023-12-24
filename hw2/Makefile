LLVM_CONFIG=/home/chuck/NYCU_HW/Advanced_compiler/llvm_build/bin/llvm-config

CXX=`$(LLVM_CONFIG) --bindir`/clang
CXXFLAGS=`$(LLVM_CONFIG) --cppflags` -fPIC -fno-rtti
LDFLAGS=`$(LLVM_CONFIG) --ldflags`
IRFLAGS=-Xclang -disable-O0-optnone -fno-discard-value-names -S -emit-llvm
OPT=`$(LLVM_CONFIG) --bindir`/opt

TEST=$(wildcard testbench/*.c)
TEST_LL=$(patsubst %.c, %.ll, $(notdir $(TEST)))

.PHONY: all test run clean
all: hw2.so test

test: $(TEST_LL)

hw2.so: hw2.cpp
	$(CXX) -shared -o $@ $< $(CXXFLAGS) $(LDFLAGS)

%.ll: testbench/%.c
	$(CXX) $(IRFLAGS) -o $@ $^

run: icpp.ll hw2.so
	#$(OPT) -time-passes -disable-output -load-pass-plugin=./hw2.so -passes='hw1' $<
	$(OPT) -disable-output -load-pass-plugin=./hw2.so -passes='hw2' $<

clean:
	rm -f *.o *.ll *.so
