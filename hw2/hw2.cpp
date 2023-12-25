#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ADT/SmallVector.h"
#include <iostream>
#include <cstring>
#include <algorithm>

using namespace llvm;
using namespace std;

int first_statement = 1;
int statement_id = 1;
SmallVector<string, 20> TREF;
SmallVector<string, 20> TGEN;
SmallVector<string, 20> DEP;
SmallVector<pair<string, string>, 20> TDEF;
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
			//get TREF info
			if(auto *LI = dyn_cast<LoadInst>(&I))
			{
				Value* val = LI->getPointerOperand();
				if(val->hasName())
				{		
					ref_name = val->getName().str();
					TREF.push_back(ref_name);

				}
				else
				{
					ref_name = "*" + TREF.back();
					TREF.push_back(ref_name);
				}
			}

			//first check TREF & TEQUIV co-alias, then get TGEN info
			if(auto *SI = dyn_cast<StoreInst>(&I))
			{
				//check TEQUIV whether have the pointer point to same address
				for(int i = 0; i < TREF.size(); i++)
				{
					for(int j = 0; j < TEQUIV.size(); j++)
					{
						if(TREF[i] == TEQUIV[j].first)
						{
							string TREF_add = TEQUIV[j].second;
							int flag = 0;
							for(int k = 0; k < TREF.size(); k++)
							{
								if(TREF_add == TREF[k])
								{
									flag = 1;
									break;
								}	
							}
							if(!flag)
							{
								TREF.push_back(TREF_add);
							}
						}
						else if(TREF[i] == TEQUIV[j].second)
						{
							string TREF_add = TEQUIV[j].first;
							int flag = 0;
							for(int k = 0; k < TREF.size(); k++)
							{
								if(TREF_add == TREF[k])
								{
									flag = 1;
									break;
								}	
							}
							if(!flag)
							{
								TREF.push_back(TREF_add);
							}
						}
					}
				}

				//
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

			//get DEP info
			if(auto *SI = dyn_cast<StoreInst>(&I))
			{
				//find flow dependence : intersection TREF(Si) & TDEF
				for(int i = 0; i < TDEF.size(); i++)
				{
					for(int j = 0; j < TREF.size(); j++)
					{
						if(TREF[j] == TDEF[i].first)
						{
							string flow_dep = TREF[j] + ": " + TDEF[i].second + "--->S" + to_string(statement_id);
							DEP.push_back(flow_dep);
						}
					}
				}

				//find output dependence : intersection TDEF & TGEN(Si)
				for(int i = 0; i < TDEF.size(); i++)
				{
					for(int j = 0; j < TGEN.size(); j++)
					{
						if(TGEN[j] == TDEF[i].first)
						{
							string out_dep = TGEN[j] + ": " + TDEF[i].second + "-O->S" + to_string(statement_id);
							DEP.push_back(out_dep);
						}
					}
				}
			}

			//Update TDEF info
			if(auto *SI = dyn_cast<StoreInst>(&I))
			{
				SmallVector<int, 20> delete_pos;
				SmallVector<pair<string, string>, 20> next_TDEF;
				for(int i = 0; i < TGEN.size(); i++)
				{
					for(int j = 0; j < TDEF.size(); j++)
					{
						//pointer point to the TGEN's element need to be remove
						if(TGEN[i] == TDEF[j].first)
						{
							delete_pos.push_back(j);
						}
					}
				}
				std::sort(delete_pos.begin(), delete_pos.end());
				for(int i = 0, j = 0; i < TDEF.size(); i++)
				{
					if(j < delete_pos.size())
					{
						if(i != delete_pos[j])
						{
							next_TDEF.push_back(TDEF[i]);
						}
						else
						{
							j++;
						}
					}
					else
					{
						next_TDEF.push_back(TDEF[i]);
					}
				}
				TDEF = next_TDEF;

				for(int i = 0; i < TGEN.size(); i++)
				{
					TDEF.push_back(pair{TGEN[i], "S" + to_string(statement_id)});
				}

				//check TEQUIV whether have the pointer point to same address
				for(int i = 0; i < TEQUIV.size(); i++)
				{
					if(gen_name == TEQUIV[i].first)
					{
						for(int j = 0; j < TDEF.size(); j++)	//update TDEF
						{
							if(TEQUIV[i].second == TDEF[j].first)
							{
								TDEF[j].second = "S" + to_string(statement_id);
							}
						}
					}
					else if(gen_name == TEQUIV[i].second)
					{
						for(int j = 0; j < TDEF.size(); j++)	//update TDEF
						{
							if(TEQUIV[i].first == TDEF[j].first)
							{
								TDEF[j].second = "S" + to_string(statement_id);
							}
						}
					}
				}
			}

			//Update TEQUIV info
			if(auto *SI = dyn_cast<StoreInst>(&I))
			{
				Value* val = SI->getOperand(0);
				if(val->getType()->getTypeID() == llvm::Type::PointerTyID)	//RHS is a pointer
				{
					SmallVector<int, 20> delete_pos;
					SmallVector<pair<string, string>, 20> next_TEQUIV;
					for(int i = 0; i < TGEN.size(); i++)
					{
						for(int j = 0; j < TEQUIV.size(); j++)
						{
							//pointer point to the TGEN's element need to be remove
							if(("*" + TGEN[i]) == TEQUIV[j].first)
							{
								delete_pos.push_back(j);
							}
						}
					}
					std::sort(delete_pos.begin(), delete_pos.end());
					for(int i = 0, j = 0; i < TEQUIV.size(); i++)
					{
						if(j < delete_pos.size())
						{
							if(i != delete_pos[j])
							{
								next_TEQUIV.push_back(TEQUIV[i]);
							}
							else
							{
								j++;
							}
						}
						else
						{
							next_TEQUIV.push_back(TEQUIV[i]);
						}
					}
					TEQUIV = next_TEQUIV;

					for(int i = 0; i < TGEN.size(); i++)
					{
						string LHS_pointer_name = "*" + TGEN[i];
						string RHS_pointer_name = val->getName().str();
						TEQUIV.push_back(pair{LHS_pointer_name, RHS_pointer_name});
					}

					int TEQUIV_size = TEQUIV.size();
					for(int i = 0; i < TEQUIV_size - 1; i++)
					{
						for(int j = i + 1; j < TEQUIV_size; j++)
						{
							string left = TEQUIV[i].first.substr(1);
							string right = TEQUIV[i].second;
							if(left == TEQUIV[j].second)
							{
								TEQUIV.push_back(pair{"*" + TEQUIV[j].first, right});
							}
						}
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

				errs() << "DEP:{";
				int first_DEP = 1;
				for(int i = 0; i < DEP.size(); i++)
				{
					if(!first_DEP)
					{
						errs() << "    " << DEP[i] << "\n";
					}
					else
					{
						errs() << "\n    " << DEP[i] << "\n";
						first_DEP = 0;
					}
				}
				errs() << "}\n";

				errs() << "TDEF:{";
				int first_TDEF = 1;
				for(int i = 0; i < TDEF.size(); i++)
				{
					if(!first_TDEF)
					{
						errs() << ", (" << TDEF[i].first << ", " << TDEF[i].second << ")";
					}
					else
					{
						errs() << "(" << TDEF[i].first << ", " << TDEF[i].second << ")";
						first_TDEF = 0;
					}
				}
				errs() << "}\n";

				errs() << "TEQUIV:{";
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
				DEP.clear();
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
