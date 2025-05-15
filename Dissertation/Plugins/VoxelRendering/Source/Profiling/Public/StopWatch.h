#pragma once
#include "CoreMinimal.h"
#include "Measurement.h"
#include "History.h"

class StopWatch : Measurement {
public:
	StopWatch() : isLocked(false), isSecure(false), timeProfileStart(0.0), key(0.0), historyLog(History()){}
	bool TryStartStopWatch(){
		if (isLocked) return false;
		isLocked = true;
		timeProfileStart = FPlatformTime::Seconds();
		return true;
	}

	bool TryStartSecureStopWatch(double& inKey) {
		if (isLocked) return false;
		isLocked = true;
		isSecure = true;
		timeProfileStart = FPlatformTime::Seconds();
		inKey = timeProfileStart;
		key = timeProfileStart;
		return true;
	}

	bool CheckKeyFits(double inKey) {
		return (this->key == inKey);
	}

	bool TryGetMeasurement(double& outVal) {
		if (isSecure) return false;
		outVal = FPlatformTime::Seconds() - timeProfileStart;
		return true;
	}

	bool TryGetSecureMeasurement(double& outVal, double inKey) {
		if (!CheckKeyFits(inKey)) return false;
		outVal = FPlatformTime::Seconds() - timeProfileStart;
		return true;
	}

	void UnlockStopWatch() {
		isLocked = false;
		isSecure = false;
	}

	bool ResetStopWatch() {
		if (isSecure) return false;
		timeProfileStart = 0;
		UnlockStopWatch();
		return true;
	}

	bool ResetSecureStopWatch(double inKey) {
		if (!CheckKeyFits(inKey)) return false;
		timeProfileStart = 0;
		UnlockStopWatch();
		return true;
	}

	bool CheckStopWatchLocked(){
		return isLocked;
	}

private:
	double timeProfileStart;
	bool isLocked;
	bool isSecure;
	double key;
	History historyLog; 
};
