#pragma once
#include "CoreMinimal.h"

class StopWatch {
	StopWatch() : isLocked(false) {}

	void StartStopWatch(){
		if (isLocked) return;
	}

	float TryGetStopWatchMeasurement() {

		return 0.0f;
	}

	void LockStopWatch(){}

	bool isLocked;
};
