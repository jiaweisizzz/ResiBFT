#include "ResiBFTBasic.h"

std::string printStage(Stage stage)
{
	if (stage == STAGE_NORMAL)
	{
		return "NORMAL";
	}
	else if (stage == STAGE_GRACIOUS)
	{
		return "GRACIOUS";
	}
	else if (stage == STAGE_UNCIVIL)
	{
		return "UNCIVIL";
	}
	return "UNKNOWN";
}

std::string printVCType(VCType vcType)
{
	if (vcType == VC_ACCEPTED)
	{
		return "ACCEPTED";
	}
	else if (vcType == VC_REJECTED)
	{
		return "REJECTED";
	}
	else if (vcType == VC_TIMEOUT)
	{
		return "TIMEOUT";
	}
	return "UNKNOWN";
}

std::string printPhase(Phase phase)
{
	if (phase == PHASE_NEWVIEW)
	{
		return "NEWVIEW";
	}
	else if (phase == PHASE_PREPARE)
	{
		return "PREPARE";
	}
	return "UNKNOWN";
}