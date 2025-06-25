#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "../Types/CharacterPresetTypes.h"
#include "../Types/LocationTypes.h"
#include "CharacterPresetManager.generated.h"

class AC_IdleCharacter;

UCLASS()
class UE_IDLE_API UCharacterPresetManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // システム初期化
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // DataTable設定
    UFUNCTION(BlueprintCallable, Category = "Character Preset")
    void SetCharacterPresetDataTable(UDataTable* InDataTable);

    // プリセットデータ取得
    UFUNCTION(BlueprintCallable, Category = "Character Preset")
    FCharacterPresetDataRow GetCharacterPreset(const FString& PresetId);

    // プリセットが存在するか確認
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Preset")
    bool DoesPresetExist(const FString& PresetId) const;

    // 全プリセットID取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Preset")
    TArray<FString> GetAllPresetIds() const;

    // 敵プリセットのみ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Preset")
    TArray<FString> GetEnemyPresetIds() const;

    // プリセットからキャラクター生成
    UFUNCTION(BlueprintCallable, Category = "Character Preset", meta = (WorldContext = "WorldContextObject"))
    AC_IdleCharacter* SpawnCharacterFromPreset(
        UObject* WorldContextObject,
        const FString& PresetId,
        const FVector& SpawnLocation,
        const FRotator& SpawnRotation = FRotator::ZeroRotator,
        TSubclassOf<AC_IdleCharacter> CharacterClass = nullptr
    );

    // Location DataTable設定
    UFUNCTION(BlueprintCallable, Category = "Location")
    void SetLocationDataTable(UDataTable* InDataTable);

    // 場所データ取得
    UFUNCTION(BlueprintCallable, Category = "Location")
    FLocationDataRow GetLocationData(const FString& LocationId);

    // 場所からランダム敵取得
    UFUNCTION(BlueprintCallable, Category = "Location")
    FString GetRandomEnemyFromLocation(const FString& LocationId);

protected:
    // キャラクターの初期設定
    void InitializeCharacter(AC_IdleCharacter* Character, const FCharacterPresetDataRow& PresetData);

    // DataTableの参照（Blueprint設定可能）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    TObjectPtr<UDataTable> CharacterPresetDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    TObjectPtr<UDataTable> LocationDataTable;

private:

    // デバッグ用
    void LogCharacterPresetError(const FString& PresetId) const;
    void LogLocationError(const FString& LocationId) const;
};