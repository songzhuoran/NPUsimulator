//BusPacket.cpp
//
//Class file for bus packet object
//

#include "BusPacket.h"

using namespace DRAMSim;
using namespace std;

BusPacket::BusPacket(BusPacketType packtype, uint64_t physicalAddr, 
		unsigned col, unsigned rw, unsigned r, unsigned b, void *dat, 
		ostream &dramsim_log_) :
	dramsim_log(dramsim_log_),
	busPacketType(packtype),
	column(col),
	row(rw),
	bank(b),
	rank(r),
	physicalAddress(physicalAddr),
	data(dat)
{}

void BusPacket::print(uint64_t currentClockCycle, bool dataStart)
{
	if (this == NULL)
	{
		return;
	}

	if (VERIFICATION_OUTPUT)
	{
		switch (busPacketType)
		{
		case READ:
			cmd_verify_out << currentClockCycle << ": read ("<<rank<<","<<bank<<","<<column<<",0);"<<endl;
			break;
		case READ_P:
			cmd_verify_out << currentClockCycle << ": read ("<<rank<<","<<bank<<","<<column<<",1);"<<endl;
			break;
		case WRITE:
			cmd_verify_out << currentClockCycle << ": write ("<<rank<<","<<bank<<","<<column<<",0 , 0, 'h0);"<<endl;
			break;
		case WRITE_P:
			cmd_verify_out << currentClockCycle << ": write ("<<rank<<","<<bank<<","<<column<<",1, 0, 'h0);"<<endl;
			break;
		case ACTIVATE:
			cmd_verify_out << currentClockCycle <<": activate (" << rank << "," << bank << "," << row <<");"<<endl;
			break;
		case PRECHARGE:
			cmd_verify_out << currentClockCycle <<": precharge (" << rank << "," << bank << "," << row <<");"<<endl;
			break;
		case REFRESH:
			cmd_verify_out << currentClockCycle <<": refresh (" << rank << ");"<<endl;
			break;
		case DATA:
			//TODO: data verification?
			break;
		default:
			ERROR("Trying to print unknown kind of bus packet");
			exit(-1);
		}
	}
}
void BusPacket::print()
{
	if (this == NULL) //pointer use makes this a necessary precaution
	{
		return;
	}
	else
	{
		switch (busPacketType)
		{
		case READ:
			PRINT("BP [READ] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
			break;
		case READ_P:
			PRINT("BP [READ_P] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
			break;
		case WRITE:
			PRINT("BP [WRITE] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
			break;
		case WRITE_P:
			PRINT("BP [WRITE_P] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
			break;
		case ACTIVATE:
			PRINT("BP [ACT] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
			break;
		case PRECHARGE:
			PRINT("BP [PRE] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
			break;
		case REFRESH:
			PRINT("BP [REF] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
			break;
		case DATA:
			PRINTN("BP [DATA] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"] data["<<data<<"]=");
			printData();
			PRINT("");
			break;
		default:
			ERROR("Trying to print unknown kind of bus packet");
			exit(-1);
		}
	}
}

void BusPacket::printData() const 
{
	if (data == NULL)
	{
		PRINTN("NO DATA");
		return;
	}
	PRINTN("'" << hex);
	for (int i=0; i < 4; i++)
	{
		PRINTN(((uint64_t *)data)[i]);
	}
	PRINTN("'" << dec);
}
