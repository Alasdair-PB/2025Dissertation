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
    int depth = 2; 
    int voxelsPerAxis = 5;
    float scale = 400.0f;
    int size = voxelsPerAxis * (1 << depth); // 20
    DispatchIsoBuffer(size, depth, scale, voxelsPerAxis);
}

void UVoxelGeneratorComponent::InitVoxelMesh(int size, int depth, float scale, int voxelsPerAxis)
{
    UWorld* world = GetWorld();
    AVoxelBody::CreateVoxelMeshActor(world, scale, size, depth, voxelsPerAxis, isoValueBuffer, typeValueBuffer, targetEraser, targetPlayer);
}

void UVoxelGeneratorComponent::DispatchIsoBuffer(int size, int depth, float scale, int voxelsPerAxis) {
    int isoSize = size + 1; //21
    int totalBufferSize = isoSize * isoSize * isoSize;
    isoValueBuffer.Reserve(totalBufferSize);
    typeValueBuffer.Reserve(totalBufferSize);

    FPlanetGeneratorDispatchParams Params(isoSize, isoSize, isoSize);
    Params.Input.baseDepthScale = scale;
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
        [WeakThis = TWeakObjectPtr<UVoxelGeneratorComponent>(this), size, depth, scale, voxelsPerAxis](FPlanetGeneratorOutput OutputVal) {
            if (!WeakThis.IsValid()) return;
            if (IsEngineExitRequested()) return;

            WeakThis->isoValueBuffer = OutputVal.outIsoValues;
            WeakThis->typeValueBuffer = OutputVal.outTypeValues;
            WeakThis->InitVoxelMesh(size, depth, scale, voxelsPerAxis);
        });
}

void UVoxelGeneratorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

