#pragma once

#include "engine.h"

/**
Convert transformation matrix defined in UE4 defined convention to right hand z-up convention
The basis used in UE4 is left hand convention with x-forward, y-rightward, z-up
Our usual convention is right convention with x-forward, y-leftward, z-up
The change-of-basis matrix from above LHC (RHC) to RHC (LHC) is defined as:
    [1  0 0 0]
	[0 -1 0 0]
	[0  0 1 0]
	[0  0 0 1]
Details on change-of-basis matrix can be found here https://brilliant.org/wiki/change-of-basis/ 
*/
inline FTransform T_lhc2rhc(const FTransform& T_lhc)
{
	FMatrix _M(FVector(1, 0, 0), FVector(0, -1, 0), FVector(0, 0, 1), FVector(0, 0, 0));
	FTransform M(_M);
	return M * T_lhc * M.Inverse();
}

inline FTransform T_rhc2lhc(const FTransform& T_rhc)
{
	FMatrix _M(FVector(1, 0, 0), FVector(0, -1, 0), FVector(0, 0, 1), FVector(0, 0, 0));
	FTransform M(_M);
	return M * T_rhc * M.Inverse();
}

class FAsyncRecord
{
public:
	bool bIsCompleted;
	FString msg;
	static TArray<FAsyncRecord*> Records; // Tracking all async task in the system
	static FAsyncRecord* Create()
	{
		FAsyncRecord* Record = new FAsyncRecord();
		return Record;
	}
	void Destory()
	{
		delete this;
	}
};