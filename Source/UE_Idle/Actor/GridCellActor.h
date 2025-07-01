#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameplayTagContainer.h"
#include "Materials/MaterialInterface.h"
#include "GridCellActor.generated.h"

/**
 * グリッドセル表示アクター
 * 2D表示システムの一部として、各グリッドセルを視覚的に表現
 */
UCLASS()
class UE_IDLE_API AGridCellActor : public AActor
{
    GENERATED_BODY()
    
public:    
    AGridCellActor();

protected:
    virtual void BeginPlay() override;

    // === 2D表示コンポーネント ===

    // セルの3Dメッシュ表示
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Cell Display")
    TObjectPtr<UStaticMeshComponent> CellMesh;

    // セルのラベル表示
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Cell Display")
    TObjectPtr<UTextRenderComponent> CellLabel;

    // === セルタイプ設定 ===

    // 現在のセルタイプ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Cell")
    FGameplayTag CurrentCellType;

    // セルタイプ別マテリアル設定
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grid Cell Materials")
    TMap<FGameplayTag, UMaterialInterface*> CellMaterials;

    // デフォルトマテリアル
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grid Cell Materials")
    TObjectPtr<UMaterialInterface> DefaultMaterial;

    // === グリッド位置 ===

    // このセルのグリッド座標
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Cell")
    FIntPoint GridPosition;

    // セルサイズ（ワールド単位）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Cell")
    float CellSize = 1000.0f; // 10m

public:
    // === セルタイプ設定 ===

    // セルタイプを設定（マテリアル・ラベル更新）
    UFUNCTION(BlueprintCallable, Category = "Grid Cell")
    void SetCellType(FGameplayTag LocationType);

    // セルタイプ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid Cell")
    FGameplayTag GetCellType() const { return CurrentCellType; }

    // === グリッド位置設定 ===

    // グリッド位置設定
    UFUNCTION(BlueprintCallable, Category = "Grid Cell")
    void SetGridPosition(const FIntPoint& NewGridPosition);

    // グリッド位置取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid Cell")
    FIntPoint GetGridPosition() const { return GridPosition; }

    // === 表示設定 ===

    // ラベル表示切り替え
    UFUNCTION(BlueprintCallable, Category = "Grid Cell")
    void SetLabelVisible(bool bVisible);

    // ラベルテキスト設定
    UFUNCTION(BlueprintCallable, Category = "Grid Cell")
    void SetLabelText(const FString& NewText);

    // マテリアル直接設定
    UFUNCTION(BlueprintCallable, Category = "Grid Cell")
    void SetCellMaterial(UMaterialInterface* NewMaterial);

    // === デバッグ情報 ===

    // セル情報表示切り替え
    UFUNCTION(BlueprintCallable, Category = "Grid Cell Debug")
    void SetDebugInfoVisible(bool bVisible);

private:
    // === 内部実装 ===

    // コンポーネント初期化
    void InitializeComponents();

    // マテリアル更新
    void UpdateMaterial();

    // ラベル更新
    void UpdateLabel();

    // グリッド座標からワールド座標計算
    FVector CalculateWorldPositionFromGrid() const;

    // デバッグ情報表示フラグ
    UPROPERTY()
    bool bShowDebugInfo = false;
};