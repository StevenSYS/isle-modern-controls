#ifndef MXDSPARALLELACTION_H
#define MXDSPARALLELACTION_H

#include "mxdsmultiaction.h"

// VTABLE: LEGO1 0x100dcf80
// VTABLE: BETA10 0x101c2988
// SIZE 0x9c
class MxDSParallelAction : public MxDSMultiAction {
public:
	MxDSParallelAction();
	~MxDSParallelAction() override;

	void CopyFrom(MxDSParallelAction& p_dsParallelAction);
	MxDSParallelAction& operator=(MxDSParallelAction& p_dsParallelAction);

	// FUNCTION: LEGO1 0x100caf00
	// FUNCTION: BETA10 0x1015b3a0
	const char* ClassName() const override // vtable+0x0c
	{
		// STRING: LEGO1 0x10102608
		return "MxDSParallelAction";
	}

	// FUNCTION: LEGO1 0x100caf10
	MxBool IsA(const char* p_name) const override // vtable+0x10
	{
		return !strcmp(p_name, MxDSParallelAction::ClassName()) || MxDSMultiAction::IsA(p_name);
	}

	// SYNTHETIC: LEGO1 0x100cb020
	// MxDSParallelAction::`scalar deleting destructor'

	MxLong GetDuration() override; // vtable+24;

	// FUNCTION: LEGO1 0x100caef0
	void SetDuration(MxLong p_duration) override { m_duration = p_duration; } // vtable+0x28

	MxDSAction* Clone() override; // vtable+2c;
};

#endif // MXDSPARALLELACTION_H
