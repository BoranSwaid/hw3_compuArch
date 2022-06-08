#include "dflow_calc.h"
#include <vector>
#include <iostream>

using namespace std;

class progEntry {
public:
	const InstInfo* inst;
	std::vector<int> dependencies;//saves the indexes of inst dependecies
	unsigned int depth;//the depth of this inst with its latency
		
};
	
class Prog {
public:
	std::vector<progEntry> prog_inst;//saves informations about each inst
	const unsigned int* latencies;
	std::vector<int> register_file;//saves the registers that we wrote to it
	unsigned int numberOfInst;

	Prog(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts);
	~Prog() = default;
	void ProgAnalyzeProg(const InstInfo progTrace[]);
	int ProgGetInstDepth(unsigned int theInst);
	int ProgGetInstDeps(unsigned int theInst, int* src1DepInst, int* src2DepInst);
	int ProgGetProgDepth();
};

int max(int num1, int num2) {
	return num1 > num2 ? num1 : num2;
}

Prog::Prog(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) : prog_inst(numOfInsts + 2), 
	latencies(opsLatency), register_file(32), numberOfInst(numOfInsts)
{
	//initiate Entry
	prog_inst[0].inst = nullptr;
	prog_inst[0].depth = 0;
	
	//initiate Exit
	prog_inst[numOfInsts + 1].inst = nullptr;
	prog_inst[numOfInsts + 1].depth = 0;

	for (int i = 1; i < numOfInsts + 1; i++) {
		prog_inst[i].inst = &progTrace[i - 1];
		prog_inst[i].depth = 0;
	}
	
	for(int i = 0; i < 32; i++){
		register_file[i] = -1;
	}
}

void Prog::ProgAnalyzeProg(const InstInfo progTrace[])
{
	//this array helps us recognizing the Exit dependencies
	bool* ExitDep = new bool[numberOfInst];
	for(int i = 0; i < numberOfInst; i++){
		ExitDep[i] = false;
	}

	for (int i = 1; i < numberOfInst + 1; i++) {
		prog_inst[i].dependencies.push_back(register_file[progTrace[i - 1].src1Idx]);
		prog_inst[i].dependencies.push_back(register_file[progTrace[i - 1].src2Idx]); 
		register_file[prog_inst[i].inst->dstIdx] = i;
		int lat1 = 0, lat2 = 0;
		if(prog_inst[i].dependencies[0] != -1) lat1 = prog_inst[prog_inst[i].dependencies[0]].depth;
		if(prog_inst[i].dependencies[1] != -1) lat2 = prog_inst[prog_inst[i].dependencies[1]].depth;
		prog_inst[i].depth += (latencies[prog_inst[i].inst->opcode] + max(lat1, lat2));
	}

	//check wich insts are not exit dependencies
	for (int i = 1; i < numberOfInst+ 1; i++) {
		if(prog_inst[i].dependencies[0] != -1) ExitDep[prog_inst[i].dependencies[0]-1] = true;
		if(prog_inst[i].dependencies[1] != -1) ExitDep[prog_inst[i].dependencies[1]-1] = true;
	}
	//update the Exit dependencies
	for (int j = 1; j < numberOfInst + 1; j++) {
		if (!ExitDep[j - 1]) {
			prog_inst[numberOfInst + 1].dependencies.push_back(j);
		}
	}
	
	//calculate the max depth of exit
	int max_depth = 0;
	for(int i = 0; i < prog_inst[numberOfInst + 1].dependencies.size(); i++){
		if(prog_inst[prog_inst[numberOfInst + 1].dependencies[i]].depth > max_depth){
			max_depth = prog_inst[prog_inst[numberOfInst + 1].dependencies[i]].depth;
		}
	}
	prog_inst[numberOfInst + 1].depth = max_depth;
	delete ExitDep;
}

int Prog::ProgGetInstDepth(unsigned int theInst)
{
	if (theInst < 0 || theInst > numberOfInst) {
		return -1;
	}
	return prog_inst[theInst + 1].depth - latencies[prog_inst[theInst + 1].inst->opcode];//depth of inst without its latency
}

int Prog::ProgGetInstDeps(unsigned int theInst, int* src1DepInst, int* src2DepInst)
{
	if (!src1DepInst || !src2DepInst || theInst < 0 || theInst >= numberOfInst) {
		return -1;
	}
	*src1DepInst = (prog_inst[theInst + 1].dependencies[0] != -1) ? prog_inst[theInst + 1].dependencies[0] - 1 : -1;
	*src2DepInst = (prog_inst[theInst + 1].dependencies[1] != -1) ? prog_inst[theInst + 1].dependencies[1] - 1 : -1;
	return 0;
}

int Prog::ProgGetProgDepth()
{
	return prog_inst[numberOfInst + 1].depth;
}

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
	Prog* myProg = new Prog(opsLatency, progTrace, numOfInsts);
	myProg->ProgAnalyzeProg(progTrace);
	return myProg;
	return PROG_CTX_NULL;
}

void freeProgCtx(ProgCtx ctx) {
	Prog* myProg = (Prog*)ctx;
	delete myProg;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
	Prog* myProg = (Prog*)ctx;
	return myProg->ProgGetInstDepth(theInst);
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int* src1DepInst, int* src2DepInst) {

	Prog* myProg = (Prog*)ctx;
	return myProg->ProgGetInstDeps(theInst, src1DepInst, src2DepInst);
}

int getProgDepth(ProgCtx ctx) {
	Prog* myProg = (Prog*)ctx;
	return myProg->ProgGetProgDepth();
}



