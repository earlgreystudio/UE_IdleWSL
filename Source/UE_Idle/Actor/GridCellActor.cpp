#include "GridCellActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

AGridCellActor::AGridCellActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // RootComponentを設定
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // セルメッシュコンポーネント作成
    CellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CellMesh"));
    CellMesh->SetupAttachment(RootComponent);

    // ラベルコンポーネント作成
    CellLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CellLabel"));
    CellLabel->SetupAttachment(RootComponent);

    // 初期設定
    InitializeComponents();
}

void AGridCellActor::BeginPlay()
{
    Super::BeginPlay();
    
    // 初期位置設定
    SetActorLocation(CalculateWorldPositionFromGrid());
    
    // 初期表示更新
    UpdateMaterial();
    UpdateLabel();

    UE_LOG(LogTemp, Log, TEXT("GridCellActor: Initialized at grid position (%d, %d)"), 
        GridPosition.X, GridPosition.Y);
}

void AGridCellActor::InitializeComponents()
{
    if (CellMesh)
    {
        // デフォルトのキューブメッシュを設定
        static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube"));
        if (CubeMeshAsset.Succeeded())
        {
            CellMesh->SetStaticMesh(CubeMeshAsset.Object);
        }

        // スケール設定（平らなセルに）
        CellMesh->SetRelativeScale3D(FVector(CellSize / 100.0f, CellSize / 100.0f, 0.1f)); // 薄い板状

        // コリジョン設定
        CellMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        CellMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
        CellMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    }

    if (CellLabel)
    {
        // ラベル設定
        CellLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f)); // セルの上に表示
        CellLabel->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); // 真上から見えるように
        CellLabel->SetText(FText::FromString(TEXT("Cell")));
        CellLabel->SetTextMaterial(nullptr); // デフォルトマテリアル使用
        CellLabel->SetHorizontalAlignment(EHTA_Center);
        CellLabel->SetVerticalAlignment(EVRTA_TextCenter);
        CellLabel->SetWorldSize(200.0f); // 文字サイズ
        CellLabel->SetVisibility(false); // 初期は非表示
    }

    // デフォルト値設定
    GridPosition = FIntPoint(0, 0);
    CurrentCellType = FGameplayTag::EmptyTag;
    CellSize = 1000.0f;
}

void AGridCellActor::SetCellType(FGameplayTag LocationType)
{
    CurrentCellType = LocationType;
    
    // マテリアルとラベルを更新
    UpdateMaterial();
    UpdateLabel();

    UE_LOG(LogTemp, VeryVerbose, TEXT("GridCellActor: Set cell type to %s at (%d, %d)"), 
        *LocationType.ToString(), GridPosition.X, GridPosition.Y);
}

void AGridCellActor::SetGridPosition(const FIntPoint& NewGridPosition)
{
    GridPosition = NewGridPosition;
    
    // ワールド位置を更新
    SetActorLocation(CalculateWorldPositionFromGrid());
    
    // ラベル更新（位置情報含む）
    UpdateLabel();

    UE_LOG(LogTemp, VeryVerbose, TEXT("GridCellActor: Moved to grid position (%d, %d)"), 
        GridPosition.X, GridPosition.Y);
}

void AGridCellActor::SetLabelVisible(bool bVisible)
{
    if (CellLabel)
    {
        CellLabel->SetVisibility(bVisible);
    }
}

void AGridCellActor::SetLabelText(const FString& NewText)
{
    if (CellLabel)
    {
        CellLabel->SetText(FText::FromString(NewText));
    }
}

void AGridCellActor::SetCellMaterial(UMaterialInterface* NewMaterial)
{
    if (CellMesh && NewMaterial)
    {
        CellMesh->SetMaterial(0, NewMaterial);
    }
}

void AGridCellActor::SetDebugInfoVisible(bool bVisible)
{
    bShowDebugInfo = bVisible;
    UpdateLabel();
}

// === Private Implementation ===

void AGridCellActor::UpdateMaterial()
{
    if (!CellMesh)
    {
        return;
    }

    UMaterialInterface* MaterialToUse = DefaultMaterial;

    // セルタイプ別マテリアル検索
    if (!CurrentCellType.IsValid())
    {
        // デフォルトマテリアル使用
    }
    else if (UMaterialInterface** FoundMaterial = CellMaterials.Find(CurrentCellType))
    {
        MaterialToUse = *FoundMaterial;
    }

    // マテリアル適用
    if (MaterialToUse)
    {
        CellMesh->SetMaterial(0, MaterialToUse);
    }
}

void AGridCellActor::UpdateLabel()
{
    if (!CellLabel)
    {
        return;
    }

    FString LabelText;

    if (bShowDebugInfo)
    {
        // デバッグ情報表示
        LabelText = FString::Printf(TEXT("(%d,%d)\\n%s"), 
            GridPosition.X, 
            GridPosition.Y,
            CurrentCellType.IsValid() ? *CurrentCellType.GetTagName().ToString() : TEXT("Empty"));
    }
    else if (CurrentCellType.IsValid())
    {
        // セルタイプ名を表示
        FString TypeName = CurrentCellType.GetTagName().ToString();
        
        // "Location.Forest" → "Forest" に変換
        FString SimplifiedName;
        if (TypeName.Split(TEXT("."), nullptr, &SimplifiedName))
        {
            LabelText = SimplifiedName;
        }
        else
        {
            LabelText = TypeName;
        }
    }
    else
    {
        LabelText = TEXT("");
    }

    CellLabel->SetText(FText::FromString(LabelText));
}

FVector AGridCellActor::CalculateWorldPositionFromGrid() const
{
    // グリッド座標をワールド座標に変換
    float WorldX = GridPosition.X * CellSize;
    float WorldY = GridPosition.Y * CellSize;
    
    // Z座標は地面レベル
    float WorldZ = 0.0f;
    
    return FVector(WorldX, WorldY, WorldZ);
}