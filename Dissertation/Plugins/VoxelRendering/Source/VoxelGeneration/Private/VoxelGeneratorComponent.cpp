#include "VoxelGeneratorComponent.h"
#include "MySimpleComputeShader.h"
#include "AVoxelBody.h"
#include "Logging/LogMacros.h"

UVoxelGeneratorComponent::UVoxelGeneratorComponent() {
	PrimaryComponentTick.bCanEverTick = true;
}

void UVoxelGeneratorComponent::BeginPlay()
{
	Super::BeginPlay();
    InitIsoDispatch();
}

void UVoxelGeneratorComponent::BeginDestroy() {
    Super::BeginDestroy();
    delete stopWatch;
}

void UVoxelGeneratorComponent::InitIsoDispatch() {
    int size = voxelsPerAxis * (1 << depth);
    DispatchIsoBuffer(size, depth, scale, voxelsPerAxis);
}

void UVoxelGeneratorComponent::InitVoxelMesh(int inSize, int inDepth, float inScale, int inVoxelsPerAxis)
{
    UWorld* world = GetWorld();
    AVoxelBody::CreateVoxelMeshActor(world, inScale, inSize, inDepth, inVoxelsPerAxis, isoValueBuffer, typeValueBuffer, targetEraser, targetPlayer, pointer);
}

void UVoxelGeneratorComponent::DispatchIsoBuffer(int inSize, int inDepth, float inScale, int inVoxelsPerAxis) {
    int isoSize = inSize + 1;
    int totalBufferSize = isoSize * isoSize * isoSize;
    isoValueBuffer.Reserve(totalBufferSize);
    typeValueBuffer.Reserve(totalBufferSize);

    FPlanetGeneratorDispatchParams Params(isoSize, isoSize, isoSize);
    Params.Input.baseDepthScale = inScale;
    Params.Input.size = isoSize;
    Params.Input.isoLevel = isoLevel;
    Params.Input.planetScaleRatio = planetScaleRatio;
    Params.Input.seed = 0;
    Params.Input.surfaceLayers = surfaceLayers;
    Params.Input.fbmAmplitude = fbmAmplitude;
    Params.Input.fbmFrequency = fbmFrequency;
    Params.Input.voronoiScale = voronoiScale;
    Params.Input.voronoiJitter = voronoiJitter;
    Params.Input.voronoiWeight = voronoiWeight;
    Params.Input.fbmWeight = fbmWeight;
    Params.Input.surfaceWeight = surfaceWeight;
    Params.Input.voronoiThreshold = voronoiThreshold;

    FPlanetGeneratorInterface::Dispatch(Params,
        [WeakThis = TWeakObjectPtr<UVoxelGeneratorComponent>(this), inSize, inDepth, inScale, inVoxelsPerAxis](FPlanetGeneratorOutput OutputVal) {
            if (!WeakThis.IsValid()) return;
            if (IsEngineExitRequested()) return;

            WeakThis->isoValueBuffer = OutputVal.outIsoValues;
            WeakThis->typeValueBuffer = OutputVal.outTypeValues;
            WeakThis->InitVoxelMesh(inSize, inDepth, inScale, inVoxelsPerAxis);
        });
}

void UVoxelGeneratorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}