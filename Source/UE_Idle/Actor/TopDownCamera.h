#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "TopDownCamera.generated.h"

/**
 * 2Dトップダウンカメラ
 * アイドルゲーム用の真上から見下ろすカメラシステム
 */
UCLASS()
class UE_IDLE_API ATopDownCamera : public AActor
{
    GENERATED_BODY()
    
public:    
    ATopDownCamera();

protected:
    virtual void BeginPlay() override;

    // === カメラコンポーネント ===

    // SpringArmコンポーネント（カメラ位置制御用）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USpringArmComponent> SpringArm;

    // メインカメラ
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> Camera;

    // === カメラ設定 ===

    // カメラの高さ（Z座標）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
    float CameraHeight = 5000.0f;

    // 視野幅（直交投影）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
    float OrthoWidth = 10000.0f;

    // カメラの角度（通常は真下向き-90度）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
    float CameraPitch = -90.0f;

    // === グリッド追跡設定 ===

    // グリッドの中心位置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    FIntPoint GridCenter = FIntPoint(10, 10);

    // グリッドセルサイズ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    float GridCellSize = 1000.0f;

    // === カメラ移動設定 ===

    // カメラ移動速度
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Settings")
    float MovementSpeed = 1000.0f;

    // ズーム速度
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Settings")
    float ZoomSpeed = 100.0f;

    // 最小ズーム（視野幅）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Settings")
    float MinOrthoWidth = 2000.0f;

    // 最大ズーム（視野幅）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Settings")
    float MaxOrthoWidth = 20000.0f;

public:
    // === カメラ制御 ===

    // グリッド座標に移動
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void MoveToGridPosition(const FIntPoint& GridPosition);

    // ワールド座標に移動
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void MoveToWorldPosition(const FVector& WorldPosition);

    // グリッドセンターに移動
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void MoveToGridCenter();

    // === ズーム制御 ===

    // ズームイン
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void ZoomIn(float ZoomAmount = 1000.0f);

    // ズームアウト
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void ZoomOut(float ZoomAmount = 1000.0f);

    // ズームレベル設定
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void SetZoomLevel(float NewOrthoWidth);

    // === カメラ設定更新 ===

    // カメラ設定を更新
    UFUNCTION(BlueprintCallable, Category = "Camera Settings")
    void UpdateCameraSettings();

    // 直交投影の切り替え
    UFUNCTION(BlueprintCallable, Category = "Camera Settings")
    void SetOrthographicProjection(bool bOrthographic);

    // === 情報取得 ===

    // 現在のズームレベル取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Camera Info")
    float GetCurrentZoomLevel() const;

    // カメラの中心グリッド座標取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Camera Info")
    FIntPoint GetCenterGridPosition() const;

    // 画面中心のワールド座標取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Camera Info")
    FVector GetCenterWorldPosition() const;

private:
    // === 内部実装 ===

    // コンポーネント初期化
    void InitializeComponents();

    // カメラ位置更新
    void UpdateCameraPosition();

    // グリッド座標→ワールド座標変換
    FVector GridToWorldPosition(const FIntPoint& GridPos) const;

    // ワールド座標→グリッド座標変換
    FIntPoint WorldToGridPosition(const FVector& WorldPos) const;

    // ズーム値の制限
    float ClampZoomLevel(float ZoomLevel) const;
};