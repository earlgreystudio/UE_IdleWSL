#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "C_GameInstance.generated.h"

/**
 * カスタムGameInstanceクラス
 * DataTableの初期化とサブシステムの設定を管理
 */
UCLASS()
class UE_IDLE_API UC_GameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UC_GameInstance();

    // 初期化
    virtual void Init() override;
    virtual void Shutdown() override;

protected:
    // DataTableアセットの参照（エディタで設定可能）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data Tables")
    class UDataTable* ItemDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data Tables")
    class UDataTable* CharacterPresetDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data Tables")
    class UDataTable* LocationDataTable;

private:
    // DataTableの初期化処理
    void InitializeDataTables();
    
    // 初期化完了フラグ
    bool bIsInitialized;
};