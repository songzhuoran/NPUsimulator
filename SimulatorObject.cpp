//SimulatorObject.cpp
//
//Base class for all classes in the simulator
//

#include <cstdlib>
#include <iostream>
#include "SimulatorObject.h"

using namespace DRAMSim;
using namespace std;

void SimulatorObject::step()
{
	// std::cout<<"currentClockCycle = "<<currentClockCycle<<std::endl;
	currentClockCycle = currentClockCycle + 1;
}


