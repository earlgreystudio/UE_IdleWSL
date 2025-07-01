#include "TopDownCamera.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/Engine.h"

ATopDownCamera::ATopDownCamera()
{
    PrimaryActorTick.bCanEverTick = false;

    // RootComponentを設定
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // SpringArmコンポーネント作成
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);

    // カメラコンポーネント作成
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);

    // 初期設定
    InitializeComponents();
}

void ATopDownCamera::BeginPlay()
{
    Super::BeginPlay();
    
    // 初期位置設定
    MoveToGridCenter();
    
    // カメラ設定適用
    UpdateCameraSettings();

    UE_LOG(LogTemp, Log, TEXT("TopDownCamera: Initialized at height %.1f with ortho width %.1f"), 
        CameraHeight, OrthoWidth);
}

void ATopDownCamera::InitializeComponents()
{
    if (SpringArm)
    {
        // SpringArm設定
        SpringArm->TargetArmLength = CameraHeight;
        SpringArm->SetRelativeRotation(FRotator(CameraPitch, 0.0f, 0.0f));
        SpringArm->bDoCollisionTest = false; // コリジョン無効
        SpringArm->bInheritPitch = false;
        SpringArm->bInheritYaw = false;
        SpringArm->bInheritRoll = false;
    }

    if (Camera)
    {
        // 直交投影設定
        Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
        Camera->OrthoWidth = OrthoWidth;
        Camera->OrthoNearClipPlane = 1.0f;
        Camera->OrthoFarClipPlane = 20000.0f;
        
        // カメラの向き設定（真下を向く）
        Camera->SetRelativeRotation(FRotator::ZeroRotator);
    }
}

void ATopDownCamera::MoveToGridPosition(const FIntPoint& GridPosition)
{
    FVector WorldPosition = GridToWorldPosition(GridPosition);
    MoveToWorldPosition(WorldPosition);

    UE_LOG(LogTemp, VeryVerbose, TEXT("TopDownCamera: Moved to grid position (%d, %d)"), 
        GridPosition.X, GridPosition.Y);
}

void ATopDownCamera::MoveToWorldPosition(const FVector& WorldPosition)
{
    // Z座標は維持（高度は変更しない）
    FVector NewLocation = FVector(WorldPosition.X, WorldPosition.Y, GetActorLocation().Z);
    SetActorLocation(NewLocation);

    UE_LOG(LogTemp, VeryVerbose, TEXT("TopDownCamera: Moved to world position (%.1f, %.1f, %.1f)"), 
        NewLocation.X, NewLocation.Y, NewLocation.Z);
}

void ATopDownCamera::MoveToGridCenter()
{
    MoveToGridPosition(GridCenter);
    
    UE_LOG(LogTemp, Log, TEXT("TopDownCamera: Moved to grid center (%d, %d)"), 
        GridCenter.X, GridCenter.Y);
}

void ATopDownCamera::ZoomIn(float ZoomAmount)
{
    float NewOrthoWidth = OrthoWidth - ZoomAmount;
    SetZoomLevel(NewOrthoWidth);
}

void ATopDownCamera::ZoomOut(float ZoomAmount)
{
    float NewOrthoWidth = OrthoWidth + ZoomAmount;
    SetZoomLevel(NewOrthoWidth);
}

void ATopDownCamera::SetZoomLevel(float NewOrthoWidth)
{
    OrthoWidth = ClampZoomLevel(NewOrthoWidth);
    
    if (Camera)
    {
        Camera->OrthoWidth = OrthoWidth;
    }

    UE_LOG(LogTemp, VeryVerbose, TEXT("TopDownCamera: Zoom level set to %.1f"), OrthoWidth);
}

void ATopDownCamera::UpdateCameraSettings()
{
    if (SpringArm)
    {
        SpringArm->TargetArmLength = CameraHeight;
        SpringArm->SetRelativeRotation(FRotator(CameraPitch, 0.0f, 0.0f));
    }

    if (Camera)
    {
        Camera->OrthoWidth = OrthoWidth;
        Camera->OrthoNearClipPlane = 1.0f;
        Camera->OrthoFarClipPlane = CameraHeight + 5000.0f; // 高度に応じて調整
    }

    UE_LOG(LogTemp, Log, TEXT("TopDownCamera: Settings updated - Height: %.1f, OrthoWidth: %.1f, Pitch: %.1f"), 
        CameraHeight, OrthoWidth, CameraPitch);
}

void ATopDownCamera::SetOrthographicProjection(bool bOrthographic)
{
    if (Camera)
    {
        Camera->ProjectionMode = bOrthographic ? 
            ECameraProjectionMode::Orthographic : 
            ECameraProjectionMode::Perspective;
        
        UE_LOG(LogTemp, Log, TEXT("TopDownCamera: Projection mode set to %s"), 
            bOrthographic ? TEXT("Orthographic") : TEXT("Perspective"));
    }
}

float ATopDownCamera::GetCurrentZoomLevel() const
{
    return OrthoWidth;
}

FIntPoint ATopDownCamera::GetCenterGridPosition() const
{
    FVector CenterWorld = GetCenterWorldPosition();
    return WorldToGridPosition(CenterWorld);
}

FVector ATopDownCamera::GetCenterWorldPosition() const
{
    FVector ActorLocation = GetActorLocation();
    // Z座標は地面レベルに設定
    return FVector(ActorLocation.X, ActorLocation.Y, 0.0f);
}

// === Private Implementation ===

void ATopDownCamera::UpdateCameraPosition()
{
    if (SpringArm)
    {
        SpringArm->TargetArmLength = CameraHeight;
    }
}

FVector ATopDownCamera::GridToWorldPosition(const FIntPoint& GridPos) const
{
    float WorldX = GridPos.X * GridCellSize;
    float WorldY = GridPos.Y * GridCellSize;
    return FVector(WorldX, WorldY, 0.0f);
}

FIntPoint ATopDownCamera::WorldToGridPosition(const FVector& WorldPos) const
{
    int32 GridX = FMath::RoundToInt(WorldPos.X / GridCellSize);
    int32 GridY = FMath::RoundToInt(WorldPos.Y / GridCellSize);
    return FIntPoint(GridX, GridY);
}

float ATopDownCamera::ClampZoomLevel(float ZoomLevel) const
{
    return FMath::Clamp(ZoomLevel, MinOrthoWidth, MaxOrthoWidth);
}