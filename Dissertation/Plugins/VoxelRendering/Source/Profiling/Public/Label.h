#pragma once
#include "CoreMinimal.h"

class Label {
public:
	FString GetTitle() {
		return labelText;
	}

private:
	FString labelText;
};
