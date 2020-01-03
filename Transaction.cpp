//Transaction.cpp
//
//Class file for transaction object
//	Transaction is considered requests sent from the CPU to
//	the memory controller (read, write, etc.)...

#include "Transaction.h"
#include "PrintMacros.h"

using std::endl;
using std::hex; 
using std::dec; 

namespace DRAMSim {

Transaction::Transaction(TransactionType transType, uint64_t addr, void *dat) :
	transactionType(transType),
	address(addr),
	data(dat)
{}

Transaction::Transaction(const Transaction &t)
	: transactionType(t.transactionType)
	  , address(t.address)
	  , data(NULL)
	  , timeAdded(t.timeAdded)
	  , timeReturned(t.timeReturned)
{
	#ifndef NO_STORAGE
	ERROR("Data storage is really outdated and these copies happen in an \n improper way, which will eventually cause problems. Please send an \n email to dramninjas [at] gmail [dot] com if you need data storage");
	abort(); 
	#endif
}

ostream &operator<<(ostream &os, const Transaction &t)
{
	if (t.transactionType == DATA_READ)
	{
		os<<"T [Read] [0x" << hex << t.address << "]" << dec <<endl;
	}
	else if (t.transactionType == DATA_WRITE)
	{
		os<<"T [Write] [0x" << hex << t.address << "] [" << dec << t.data << "]" <<endl;
	}
	else if (t.transactionType == RETURN_DATA)
	{
		os<<"T [Data] [0x" << hex << t.address << "] [" << dec << t.data << "]" <<endl;
	}
	return os; 
}
}

