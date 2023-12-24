#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ADT/SmallVector.h"
#include <iostream>
#include <cstring>

using namespace llvm;
using namespace std;

int first_statement = 1;
int statement_id = 1;
SmallVector<string, 20> TREF;
SmallVector<string, 20> TGEN;
SmallVector<pair<string, string>, 20> TEQUIV;

string gen_name;
string ref_name;

namespace {

struct HW2Pass : public PassInfoMixin<HW2Pass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

PreservedAnalyses HW2Pass::run(Function &F, FunctionAnalysisManager &FAM) {
	for(BasicBlock &BB : F)
	{
		//In this work, only one basicBlock
		for(Instruction &I : BB)
		{
			if(auto *SI = dyn_cast<StoreInst>(&I))	//get TGEN info
			{
				Value* val = SI->getOperand(1);
				if(val->hasName())
				{
					gen_name = val->getName().str();
				}
				else
				{
					gen_name = "*" + ref_name;	//last load inst will have the name
				}
				TGEN.push_back(gen_name);

				//check TEQUIV whether have the pointer point to same address
				for(int i = 0; i < TEQUIV.size(); i++)
				{
					if(gen_name == TEQUIV[i].first)
					{
						TGEN.push_back(TEQUIV[i].second);
					}
					else if(gen_name == TEQUIV[i].second)
					{
						TGEN.push_back(TEQUIV[i].first);
					}
				}
			}

			if(auto *SI = dyn_cast<StoreInst>(&I))	//get TEQUIV info
			{
				Value* val = SI->getOperand(0);
				if(val->getType()->getTypeID() == llvm::Type::PointerTyID)	//RHS is a pointer
				{
					string RHS_pointer_name = val->getName().str();
					string LHS_pointer_name = "*" + TGEN.back();
					TEQUIV.push_back(pair{LHS_pointer_name, RHS_pointer_name});
					
					//check other equiv
					for(int i = 0; i < TEQUIV.size() - 1; i++)
					{
						if(RHS_pointer_name == TEQUIV[i].first.substr(1))
						{
							TEQUIV.push_back(pair{"*" + LHS_pointer_name, TEQUIV[i].second});
						}
						else if(RHS_pointer_name == TEQUIV[i].second.substr(1))
						{
							TEQUIV.push_back(pair{"*" + LHS_pointer_name, TEQUIV[i].first});
						}
					}
				}
			}


			if(auto *LI = dyn_cast<LoadInst>(&I))	//get TREF info
			{
				Value* val = LI->getPointerOperand();
				ref_name = val->getName().str();
				TREF.push_back(ref_name);

				//check TEQUIV whether have the pointer point to same address
				for(int i = 0; i < TEQUIV.size(); i++)
				{
					if(ref_name == TEQUIV[i].first)
					{
						TREF.push_back("[" + TEQUIV[i].second + "]");
					}
					else if(ref_name == TEQUIV[i].second)
					{
						TREF.push_back("[" + TEQUIV[i].first + "]");
					}
				}
			}

			if(auto *SI = dyn_cast<StoreInst>(&I))	//print statement result
			{
				if(!first_statement)
				{
					errs() << "\nS" << statement_id++ << ":--------------------\n";
				}
				else
				{
					errs() << "S" << statement_id++ << ":--------------------\n";
					first_statement = 0;
				}

				errs() << "TREF: {";
				int first_TREF = 1;
				for(int i = 0; i < TREF.size(); i++)
				{
					if(!first_TREF)
					{
						errs() << ", " << TREF[i];
					}
					else
					{
						errs() << TREF[i];
						first_TREF = 0;
					}
				}
				errs() << "}\n";

				errs() << "TGEN: {";
				int first_TGEN = 1;
				for(int i = 0; i < TGEN.size(); i++)
				{
					if(!first_TGEN)
					{
						errs() << ", " << TGEN[i];
					}
					else
					{
						errs() << TGEN[i];
						first_TGEN = 0;
					}
				}
				errs() << "}\n";

				errs() << "DEP: {";
				errs() << "}\n";

				errs() << "TDEF: {";
				errs() << "}\n";

				errs() << "TEQUIV: {";
				int first_TEQUIV = 1;
				for(int i = 0; i < TEQUIV.size(); i++)
				{
					if(!first_TEQUIV)
					{
						errs() << ", (" << TEQUIV[i].first << ", " << TEQUIV[i].second << ")";
					}
					else
					{
						errs() << "(" << TEQUIV[i].first << ", " << TEQUIV[i].second << ")";
						first_TEQUIV = 0;
					}
				}
				errs() << "}\n";

				TREF.clear();
				TGEN.clear();
			}
		}
	}	
	return PreservedAnalyses::all();
}

} // end anonymous namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HW2Pass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hw2") {
                    FPM.addPass(HW2Pass());
                    return true;
                  }
                  return false;
                });
          }};
}
