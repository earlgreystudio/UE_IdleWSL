#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BattleSystemManager.generated.h"

class ULocationEventManager;
class UCombatComponent;
class AC_IdleCharacter;

/**
 * 戦闘システム全体を管理するサブシステム
 * LocationEventManagerとCombatComponentを統括
 */
UCLASS()
class UE_IDLE_API UBattleSystemManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // システム初期化
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // チームの冒険開始
    UFUNCTION(BlueprintCallable, Category = "Battle System")
    bool StartTeamAdventure(const TArray<AC_IdleCharacter*>& TeamMembers, const FString& LocationId);

    // 戦闘状態の確認
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Battle System")
    bool IsTeamInCombat(const TArray<AC_IdleCharacter*>& TeamMembers) const;

    // LocationEventManagerへのアクセス
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Battle System")
    ULocationEventManager* GetLocationEventManager() const { return LocationEventManager; }

    // CombatComponentへのアクセス
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Battle System")
    UCombatComponent* GetCombatComponent() const { return CombatComponent; }

protected:
    // 管理用Actorの作成
    void CreateBattleSystemActor();
    
    // 戦闘終了時の処理
    UFUNCTION()
    void OnCombatCompleted(const TArray<AC_IdleCharacter*>& Winners, const TArray<AC_IdleCharacter*>& Losers, float Duration);

private:
    // 戦闘システム管理用Actor
    UPROPERTY()
    class AActor* BattleSystemActor;

    // コンポーネント
    UPROPERTY()
    TObjectPtr<ULocationEventManager> LocationEventManager;

    UPROPERTY()
    TObjectPtr<UCombatComponent> CombatComponent;
};