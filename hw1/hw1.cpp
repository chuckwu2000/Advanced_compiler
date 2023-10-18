#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ADT/SmallVector.h"
#include <iostream>
#include <cstring>
#include  <stdio.h>
#include  <math.h>

using namespace llvm;
using namespace std;

typedef struct array_element
{
	int read_write;
	const char* array_name;
	int ind_var_x;
	int ind_var_y;
}array_element_type;
#include  <stdio.h>
#include  <math.h>
namespace {

struct HW1Pass : public PassInfoMixin<HW1Pass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

PreservedAnalyses HW1Pass::run(Function &F, FunctionAnalysisManager &FAM) {
	const char* ind_var_name;
	int ind_var_lowerbound = 0;
	int ind_var_upperbound = 0;

	typedef SmallVector<array_element_type, 10> array_vec_type;
	SmallVector<array_element_type, 10> array_vec;
	SmallVector<array_vec_type, 10> statement_vec;
	int statement_total = 0;
		
	for(BasicBlock &BB : F)
	{
		if(strcmp(BB.getName().data(), "entry") == 0)			//to get induction variable's name & lower_bound
		{
			for(Instruction &I : BB)
			{
				if(auto *SI = dyn_cast<StoreInst>(&I))
				{
					Value* first_operand = SI->getOperand(0);
					if(auto *constant_int = dyn_cast<ConstantInt>(first_operand))
					{
						ind_var_lowerbound = constant_int->getSExtValue();
					}
					ind_var_name = SI->getOperand(1)->getName().data();
				}
			}
		}	
		if(strncmp(BB.getName().data(), "for.cond", 8) == 0)	//to get ind_var's upper_bound
		{
			for(Instruction &I : BB)
			{
				if(auto *CI = dyn_cast<CmpInst>(&I))
				{
					Value* second_operand = CI->getOperand(1);
					if(auto *constant_int = dyn_cast<ConstantInt>(second_operand))
					{
						ind_var_upperbound = constant_int->getSExtValue();
					}
				}
			}
		}
		if(strncmp(BB.getName().data(), "for.body", 8) == 0)	//handle loop body
		{
			int ind_var_change = 0;
			int ind_var_x = 1;	//(xi + y)
			int ind_var_y = 0;

			for(Instruction &I : BB)
			{
				if(auto *LI = dyn_cast<LoadInst>(&I))			//dyn_cast<> will do "checking cast", in here return NULL if I is not LoadInst type
				{
					Value* val = LI->getPointerOperand();
					if(strcmp(ind_var_name, val->getName().data()) == 0)	//start to record ind_var's change
					{
						ind_var_change = 1;
					}
				}
				else if(auto *SI = dyn_cast<StoreInst>(&I))		//used to differentiate statement
				{
					array_vec.back().read_write = 1;			//1 for write
					statement_vec.push_back(array_vec);
					statement_total++;
					while(!array_vec.empty())					//clean array_vec for next statement to use
					{
						array_vec.pop_back();
					}
				}
				else if(auto *GEP = dyn_cast<GetElementPtrInst>(&I))	//get_element_ptr instruction
				{
					const char* array_name = I.getOperand(0)->getName().data();

					array_element_type array;
					array.read_write = 0;
					array.array_name = array_name;
					array.ind_var_x = ind_var_x;
					array.ind_var_y = ind_var_y;
					array_vec.push_back(array);

					ind_var_x = 1;
					ind_var_y = 0;
					ind_var_change = 0;
				}
				else if(ind_var_change == 1)	//need to record ind_var's change
				{
					if(strcmp(I.getOpcodeName(), "add") == 0)
					{
						Value* first_operand = I.getOperand(0);
						if(auto *constant_int = dyn_cast<ConstantInt>(first_operand))
						{
							ind_var_y += constant_int->getSExtValue();
						}
						Value* second_operand = I.getOperand(1);
						if(auto *constant_int = dyn_cast<ConstantInt>(second_operand))
						{
							ind_var_y += constant_int->getSExtValue();
						}
					}
					else if(strcmp(I.getOpcodeName(), "sub") == 0)
					{
						Value* first_operand = I.getOperand(0);
						if(auto *constant_int = dyn_cast<ConstantInt>(first_operand))
						{
							ind_var_y -= constant_int->getSExtValue();
						}
						Value* second_operand = I.getOperand(1);
						if(auto *constant_int = dyn_cast<ConstantInt>(second_operand))
						{
							ind_var_y -= constant_int->getSExtValue();
						}
					}
					else if(strcmp(I.getOpcodeName(), "mul") == 0)
					{
						Value* first_operand = I.getOperand(0);
						if(auto *constant_int = dyn_cast<ConstantInt>(first_operand))
						{
							ind_var_x *= constant_int->getSExtValue();
							ind_var_y *= constant_int->getSExtValue();
						}
						Value* second_operand = I.getOperand(1);
						if(auto *constant_int = dyn_cast<ConstantInt>(second_operand))
						{
							ind_var_x *= constant_int->getSExtValue();
							ind_var_y *= constant_int->getSExtValue();
						}
					}
				}
			}
		}
	}
/*
	for(int i=0;i<statement_total;i++)
	{
		array_vec_type statement = statement_vec[i];
		for(int j=0;j<statement.size();j++)
		{
			array_element_type array = statement[j];
			if(array.read_write == 0)
			{
				errs()<<"read : "<<array.array_name<<"["<<array.ind_var_x<<"i + "<<array.ind_var_y<<"]\n";
			}
			else
			{
				errs()<<"write : "<<array.array_name<<"["<<array.ind_var_x<<"i + "<<array.ind_var_y<<"]\n";
			}
		}
	}
*/
	/*====Flow Dependency====*/
	errs()<<"====Flow Dependency====\n";
	for(int k = 0;k < statement_total;k++)							//choose a statement
	{
		SmallVector<int, 1000> read_vec;
		array_element_type array_write = statement_vec[k].back();
		for(int i = ind_var_lowerbound;i < ind_var_upperbound;i++)	//choose an iteration
		{
			int write_val = array_write.ind_var_x * i + array_write.ind_var_y;
			for(int j = i;j < ind_var_upperbound;j++)				//only behind this iteration can have dependent
			{
				if(j == i)											//check tail of statement in this iteration
				{
					for(int s = k + 1;s < statement_total;s++)		//tail of statement
					{
						array_vec_type tmp_vec = statement_vec[s];
						for(int a = 0;a < tmp_vec.size() - 1;a++)	//the last one is assign's LHS
						{
							array_element_type array_read = tmp_vec[a];
							if(strcmp(array_read.array_name, array_write.array_name) == 0)
							{
								int read_val = array_read.ind_var_x * j + array_read.ind_var_y;
								if(read_val == write_val)
								{
									errs()<<"(i="<<i<<",i="<<j<<")\n";
									errs()<<array_write.array_name<<":S"<<k+1<<" -----> S"<<s+1<<"\n";
								}
							}
						}
					}
				}
				else
				{
					for(int s = 0;s < statement_total;s++)			//all of statement
					{
						array_vec_type tmp_vec = statement_vec[s];
						for(int a = 0;a < tmp_vec.size() - 1;a++)	//the last one is assign's LHS
						{
							array_element_type array_read = tmp_vec[a];
							if(strcmp(array_read.array_name, array_write.array_name) == 0)
							{
								int read_val = array_read.ind_var_x * j + array_read.ind_var_y;
								if(read_val == write_val)
								{
									errs()<<"(i="<<i<<",i="<<j<<")\n";
									errs()<<array_write.array_name<<":S"<<k+1<<" -----> S"<<s+1<<"\n";
								}
							}
						}
					}
				}
			}
		}
	}

	/*====Anti-Dependency====*/
	errs()<<"====Anti-Dependency====\n";

	/*====Output Dependency====*/
	errs()<<"====Output Dependency====\n";

	errs() << "[HW1]: " << F.getName() << '\n';
	return PreservedAnalyses::all();
}

} // end anonymous namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HW1Pass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hw1") {
                    FPM.addPass(HW1Pass());
                    return true;
                  }
                  return false;
                });
          }};
}
