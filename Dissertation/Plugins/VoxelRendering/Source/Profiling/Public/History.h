#pragma once
#include "CoreMinimal.h"
#include "Measurement.h"

class History {
public:
	void PushEntryToHistory(Measurement measurement) {
		historyLog.Add(measurement);
	}

	void DebugHistory() {
		for (Measurement entry : historyLog)
			UE_LOG(LogTemp, Warning, TEXT("Out measurement: %s %f"), (*entry.label.GetTitle()), entry.value);
	}

	void GraphHistory() {

	}
	
private:
	TArray<Measurement> historyLog;
};
