#pragma once
#include "CoreMinimal.h"

struct AABB {
    FVector min;
    FVector max;

    FVector Center() const {
        return (min + max) * 0.5f;
    }

    FVector Extent() const {
        return (max - min) * 0.5f;
    }

    bool Contains(const FVector& point) const {
        return (point.X >= min.X && point.X <= max.X) &&
            (point.Y >= min.Y && point.Y <= max.Y) &&
            (point.Z >= min.Z && point.Y <= max.Z);
    }
};
