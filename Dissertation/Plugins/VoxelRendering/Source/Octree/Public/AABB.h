#pragma once
#include "CoreMinimal.h"

struct AABB {
    FVector3f min;
    FVector3f max;

    FVector3f Center() const {
        return (min + max) * 0.5f;
    }

    FVector3f Extent() const {
        return (max - min) * 0.5f;
    }

    FVector3f Size() const {
        return max - min;
    }

    bool Contains(const FVector3f& point) const {
        return (point.X >= min.X && point.X <= max.X) &&
            (point.Y >= min.Y && point.Y <= max.Y) &&
            (point.Z >= min.Z && point.Y <= max.Z);
    }
};
