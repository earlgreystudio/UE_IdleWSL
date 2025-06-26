#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/CharacterTypes.h"
#include "CharacterStatusComponent.generated.h"

// デリゲート宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatusChanged, const FCharacterStatus&, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpecialtyTypeChanged, ESpecialtyType, NewSpecialtyType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTalentChanged, const FCharacterTalent&, NewTalent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterDataUpdated);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UCharacterStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCharacterStatusComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ステータス
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Status")
	FCharacterStatus Status;

	// 専門性
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Status")
	ESpecialtyType SpecialtyType;

	// 才能
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Status")
	FCharacterTalent Talent;

	// 派生ステータス（事前計算済み）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Derived Stats")
	FDerivedStats DerivedStats;

	// ステータス取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Status")
	FCharacterStatus GetStatus() const { return Status; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Status")
	float GetCurrentHealth() const { return Status.CurrentHealth; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Status")
	float GetMaxHealth() const { return Status.MaxHealth; }

	UFUNCTION(BlueprintCallable, Category = "Character Status")
	void SetStatus(const FCharacterStatus& NewStatus);

	// 専門性取得・設定
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Status")
	ESpecialtyType GetSpecialtyType() const { return SpecialtyType; }

	UFUNCTION(BlueprintCallable, Category = "Character Status")
	void SetSpecialtyType(ESpecialtyType NewSpecialtyType);

	// 才能取得・設定
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Status")
	FCharacterTalent GetTalent() const { return Talent; }

	UFUNCTION(BlueprintCallable, Category = "Character Status")
	void SetTalent(const FCharacterTalent& NewTalent);

	// 派生ステータス取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Derived Stats")
	FDerivedStats GetDerivedStats() const { return DerivedStats; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Derived Stats")
	float GetConstructionPower() const { return DerivedStats.ConstructionPower; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Derived Stats")
	float GetProductionPower() const { return DerivedStats.ProductionPower; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Derived Stats")
	float GetGatheringPower() const { return DerivedStats.GatheringPower; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Derived Stats")
	float GetCookingPower() const { return DerivedStats.CookingPower; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Derived Stats")
	float GetCraftingPower() const { return DerivedStats.CraftingPower; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Derived Stats")
	float GetAttackSpeed() const { return DerivedStats.AttackSpeed; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Derived Stats")
	float GetHitChance() const { return DerivedStats.HitChance; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Derived Stats")
	float GetDodgeChance() const { return DerivedStats.DodgeChance; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Derived Stats")
	float GetDPS() const { return DerivedStats.DPS; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Derived Stats")
	float GetCombatPower() const { return DerivedStats.CombatPower; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Derived Stats")
	float GetWorkPower() const { return DerivedStats.WorkPower; }

	// 派生ステータス再計算
	UFUNCTION(BlueprintCallable, Category = "Derived Stats")
	void RecalculateDerivedStats();

	// 装備変更時の呼び出し用
	UFUNCTION(BlueprintCallable, Category = "Derived Stats")
	void OnEquipmentChanged();

	// ステータス効果変更時の呼び出し用
	UFUNCTION(BlueprintCallable, Category = "Derived Stats")
	void OnStatusEffectChanged();

	// 体力変更（個別）
	UFUNCTION(BlueprintCallable, Category = "Character Status")
	void SetCurrentHealth(float NewHealth);

	// ======== イベントディスパッチャー ========
	
	// ステータス変更時
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnStatusChanged OnStatusChanged;

	// 体力変更時
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnHealthChanged OnHealthChanged;

	// 専門性変更時
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnSpecialtyTypeChanged OnSpecialtyTypeChanged;

	// 才能変更時
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnTalentChanged OnTalentChanged;

	// 汎用データ更新通知
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnCharacterDataUpdated OnCharacterDataUpdated;

private:
	// 派生ステータス計算関数
	void CalculateConstructionPower();
	void CalculateProductionPower();
	void CalculateGatheringPower();
	void CalculateCookingPower();
	void CalculateCraftingPower();
	void CalculateCombatStats();
	void CalculateDisplayStats();

	// ヘルパー関数
	float GetSkillValue(ESkillType SkillType) const;
	float GetEquipmentBonus(const FString& StatType) const;
	float GetStatusEffectMultiplier(const FString& StatType) const;
	FString GetEquippedWeaponId() const;
	
	// 武器・アーマー情報取得
	float GetWeaponWeight(const FString& WeaponItemId) const;
	int32 GetWeaponAttackPower(const FString& WeaponItemId) const;
	ESkillType GetWeaponSkillType(const FString& WeaponItemId) const;
	bool IsRangedWeapon(const FString& WeaponItemId) const;
	float GetArmorDefense() const;
};