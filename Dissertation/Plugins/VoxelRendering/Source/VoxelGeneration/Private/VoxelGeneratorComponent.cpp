#include "VoxelGeneratorComponent.h"
#include "MySimpleComputeShader.h"
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
    //voxelRenderer->DestroyComponent();
    delete stopWatch;
}

void UVoxelGeneratorComponent::InitIsoDispatch() {
    bounds = { FVector3f(-200.0f), FVector3f(200.0f) };
    int depth = 2; 
    int nodesPerAxisMaxRes = Octree::IntPow(2, depth);
    int size = voxelsPerAxis * nodesPerAxisMaxRes;
    DispatchIsoBuffer(size, depth);
}

void UVoxelGeneratorComponent::InitVoxelMesh(int size, int depth)
{
    AActor* Owner = GetOwner();
    voxelMesh = NewObject<UVoxelMeshComponent>(Owner);
    voxelMesh->RegisterComponent();
    voxelMesh->InitVoxelMesh(bounds, depth, isoValueBuffer, typeValueBuffer);
}

void UVoxelGeneratorComponent::DispatchIsoBuffer(int size, int depth) {
    int isoSize = size + 1;
    isoValueBuffer.Reserve(isoSize * isoSize * isoSize);
    typeValueBuffer.Reserve(isoSize * isoSize * isoSize);

    FPlanetGeneratorDispatchParams Params(isoSize, isoSize, isoSize);
    Params.Input.baseDepthScale = 400.0f;
    Params.Input.size = isoSize;
    Params.Input.isoLevel = isoLevel;
    Params.Input.planetScaleRatio = 0.9;
    Params.Input.seed = 0;

    FPlanetGeneratorInterface::Dispatch(Params,
        [WeakThis = TWeakObjectPtr<UVoxelGeneratorComponent>(this), size, depth](FPlanetGeneratorOutput OutputVal) {
            if (!WeakThis.IsValid()) return;
            if (IsEngineExitRequested()) return;

            WeakThis->isoValueBuffer = OutputVal.outIsoValues;
            WeakThis->typeValueBuffer = OutputVal.outTypeValues;
            WeakThis->InitVoxelMesh(size, depth);
        });
}

void UVoxelGeneratorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

