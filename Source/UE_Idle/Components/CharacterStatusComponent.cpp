#include "CharacterStatusComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/InventoryComponent.h"
#include "../Managers/ItemDataTableManager.h"
#include "Engine/World.h"

UCharacterStatusComponent::UCharacterStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// デフォルトステータス設定
	Status = FCharacterStatus();
	SpecialtyType = ESpecialtyType::Baseball; // デフォルト専門性
	Talent = FCharacterTalent();    // デフォルト才能
	DerivedStats = FDerivedStats(); // デフォルト派生ステータス
}

void UCharacterStatusComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 初回の派生ステータス計算
	RecalculateDerivedStats();
}

void UCharacterStatusComponent::SetStatus(const FCharacterStatus& NewStatus)
{
	Status = NewStatus;
	
	// イベント通知
	OnStatusChanged.Broadcast(NewStatus);
	OnCharacterDataUpdated.Broadcast();
}

void UCharacterStatusComponent::SetSpecialtyType(ESpecialtyType NewSpecialtyType)
{
	SpecialtyType = NewSpecialtyType;
	
	// イベント通知
	OnSpecialtyTypeChanged.Broadcast(NewSpecialtyType);
	OnCharacterDataUpdated.Broadcast();
}

void UCharacterStatusComponent::SetTalent(const FCharacterTalent& NewTalent)
{
	Talent = NewTalent;
	
	// 派生ステータス再計算
	RecalculateDerivedStats();
	
	// イベント通知
	OnTalentChanged.Broadcast(NewTalent);
	OnCharacterDataUpdated.Broadcast();
}

void UCharacterStatusComponent::SetCurrentHealth(float NewHealth)
{
	// 範囲制限
	NewHealth = FMath::Clamp(NewHealth, 0.0f, Status.MaxHealth);
	
	if (Status.CurrentHealth != NewHealth)
	{
		Status.CurrentHealth = NewHealth;
		
		// イベント通知
		OnHealthChanged.Broadcast(NewHealth);
		OnStatusChanged.Broadcast(Status);
		OnCharacterDataUpdated.Broadcast();
	}
}

// 派生ステータス関連の実装

void UCharacterStatusComponent::RecalculateDerivedStats()
{
	// 各種能力値を計算
	CalculateConstructionPower();
	CalculateProductionPower();
	CalculateGatheringPower();
	CalculateCookingPower();
	CalculateCraftingPower();
	CalculateCombatStats();
	CalculateDisplayStats();
}

void UCharacterStatusComponent::OnEquipmentChanged()
{
	UE_LOG(LogTemp, Warning, TEXT("OnEquipmentChanged called - Recalculating stats..."));
	RecalculateDerivedStats();
	UE_LOG(LogTemp, Warning, TEXT("Stats recalculated - DPS: %.2f, TotalDefensePower: %.2f"), 
		DerivedStats.DPS, DerivedStats.TotalDefensePower);
}

void UCharacterStatusComponent::OnStatusEffectChanged()
{
	RecalculateDerivedStats();
}

// 作業関連能力値計算

void UCharacterStatusComponent::CalculateConstructionPower()
{
	float BaseValue = 10.0f;
	
	// 建築スキルの影響（メイン）
	float ConstructionSkill = GetSkillValue(ESkillType::Construction);
	BaseValue += ConstructionSkill * 3.0f;
	
	// 基本能力値の影響
	BaseValue += Talent.Strength * 0.8f;      // 力の影響大
	BaseValue += Talent.Dexterity * 0.5f;     // 器用さの影響中
	BaseValue += Talent.Intelligence * 0.3f;  // 知能の影響小
	
	// 装備・ステータス効果
	BaseValue += GetEquipmentBonus("Construction");
	BaseValue *= GetStatusEffectMultiplier("Construction");
	
	DerivedStats.ConstructionPower = FMath::Max(1.0f, BaseValue);
}

void UCharacterStatusComponent::CalculateProductionPower()
{
	float BaseValue = 10.0f;
	
	// 工作スキルの影響
	float CraftingSkill = GetSkillValue(ESkillType::Crafting);
	BaseValue += CraftingSkill * 2.5f;
	
	// 基本能力値の影響
	BaseValue += Talent.Dexterity * 0.8f;     // 器用さメイン
	BaseValue += Talent.Intelligence * 0.6f;  // 知能で効率化
	BaseValue += Talent.Strength * 0.3f;      // 力でパワー
	
	// 装備・ステータス効果
	BaseValue += GetEquipmentBonus("Production");
	BaseValue *= GetStatusEffectMultiplier("Production");
	
	DerivedStats.ProductionPower = FMath::Max(1.0f, BaseValue);
}

void UCharacterStatusComponent::CalculateGatheringPower()
{
	float BaseValue = 10.0f;
	
	// サバイバルスキルの影響
	float SurvivalSkill = GetSkillValue(ESkillType::Survival);
	BaseValue += SurvivalSkill * 2.0f;
	
	// 基本能力値の影響
	BaseValue += Talent.Strength * 0.6f;      // 力で採集量
	BaseValue += Talent.Agility * 0.5f;       // 敏捷で効率
	BaseValue += Talent.Toughness * 0.4f;     // 頑丈で持続力
	
	// 装備・ステータス効果
	BaseValue += GetEquipmentBonus("Gathering");
	BaseValue *= GetStatusEffectMultiplier("Gathering");
	
	DerivedStats.GatheringPower = FMath::Max(1.0f, BaseValue);
}

void UCharacterStatusComponent::CalculateCookingPower()
{
	float BaseValue = 10.0f;
	
	// 料理スキルの影響
	float CookingSkill = GetSkillValue(ESkillType::Cooking);
	BaseValue += CookingSkill * 3.0f;
	
	// 基本能力値の影響
	BaseValue += Talent.Dexterity * 0.7f;     // 器用さメイン
	BaseValue += Talent.Intelligence * 0.5f;  // 知能でレシピ理解
	BaseValue += Talent.Agility * 0.3f;       // 敏捷で手際
	
	// 装備・ステータス効果
	BaseValue += GetEquipmentBonus("Cooking");
	BaseValue *= GetStatusEffectMultiplier("Cooking");
	
	DerivedStats.CookingPower = FMath::Max(1.0f, BaseValue);
}

void UCharacterStatusComponent::CalculateCraftingPower()
{
	float BaseValue = 10.0f;
	
	// 各種工作スキル平均
	float CraftingSkill = GetSkillValue(ESkillType::Crafting);
	float TailoringSkill = GetSkillValue(ESkillType::Tailoring);
	float EngineeringSkill = GetSkillValue(ESkillType::Engineering);
	float AverageSkill = (CraftingSkill + TailoringSkill + EngineeringSkill) / 3.0f;
	
	BaseValue += AverageSkill * 2.5f;
	
	// 基本能力値の影響
	BaseValue += Talent.Dexterity * 0.8f;     // 器用さメイン
	BaseValue += Talent.Intelligence * 0.6f;  // 知能で設計
	BaseValue += Talent.Strength * 0.2f;      // 力で材料加工
	
	// 装備・ステータス効果
	BaseValue += GetEquipmentBonus("Crafting");
	BaseValue *= GetStatusEffectMultiplier("Crafting");
	
	DerivedStats.CraftingPower = FMath::Max(1.0f, BaseValue);
}

// 戦闘関連能力値計算（CombatCalculatorの計算式を使用）

void UCharacterStatusComponent::CalculateCombatStats()
{
	FString EquippedWeapon = GetEquippedWeaponId();
	
	// 武器データを取得
	float WeaponWeight = GetWeaponWeight(EquippedWeapon);
	int32 WeaponAttackPower = GetWeaponAttackPower(EquippedWeapon);
	ESkillType WeaponSkillType = GetWeaponSkillType(EquippedWeapon);
	bool bIsRangedWeapon = IsRangedWeapon(EquippedWeapon);
	
	// 対応するスキル値を取得
	float WeaponSkill = GetSkillValue(WeaponSkillType);
	
	// 攻撃速度
	if (bIsRangedWeapon)
	{
		// 遠距離武器: 1.0 + (器用 × 0.01) + (スキルレベル × 0.03)
		DerivedStats.AttackSpeed = 1.0f + (Talent.Dexterity * 0.01f) + (WeaponSkill * 0.03f);
	}
	else
	{
		// 近接武器: BaseSpeed - WeightPenalty
		float BaseSpeed = 2.0f + (Talent.Agility * 0.02f) + (WeaponSkill * 0.01f);
		float WeightPenalty = WeaponWeight / FMath::Max(1.0f, Talent.Strength * 0.3f);
		DerivedStats.AttackSpeed = FMath::Max(0.1f, BaseSpeed - WeightPenalty);
	}
	
	// 命中率: 50 + (スキルレベル × 2) + (器用 × 1.5) - WeightPenalty
	float BaseHitChance = 50.0f + (WeaponSkill * 2.0f) + (Talent.Dexterity * 1.5f);
	float WeightPenalty = WeaponWeight / FMath::Max(1.0f, Talent.Strength * 0.5f);
	DerivedStats.HitChance = FMath::Max(5.0f, BaseHitChance - WeightPenalty);
	
	UE_LOG(LogTemp, Warning, TEXT("HitChance Calc - WeaponSkill: %.1f, Dexterity: %.1f, WeaponWeight: %.1f, Strength: %.1f"), 
		WeaponSkill, Talent.Dexterity, WeaponWeight, Talent.Strength);
	UE_LOG(LogTemp, Warning, TEXT("HitChance Calc - BaseHit: %.1f, Penalty: %.1f, Final: %.1f"), 
		BaseHitChance, WeightPenalty, DerivedStats.HitChance);
	
	// 回避率: 10 + (敏捷 × 2) + (回避スキル × 2) - 新仕様
	float EvasionSkill = GetSkillValue(ESkillType::Evasion);
	float BaseDodgeChance = 10.0f + (Talent.Agility * 2.0f) + (EvasionSkill * 2.0f);
	// TODO: 装備ペナルティの詳細実装
	// TODO: 盾装備時は回避率半減
	DerivedStats.DodgeChance = FMath::Max(0.0f, BaseDodgeChance);
	
	// 受け流し率: 5 + (器用 × 1.5) + (受け流しスキル × 3)
	float ParrySkill = GetSkillValue(ESkillType::Parry);
	float BaseParryChance = 5.0f + (Talent.Dexterity * 1.5f) + (ParrySkill * 3.0f);
	// TODO: 装備ペナルティの詳細実装
	DerivedStats.ParryChance = FMath::Max(0.0f, BaseParryChance);
	
	// 盾防御率: 新システム - 器用さ依存
	float ShieldSkill = GetSkillValue(ESkillType::Shield);
	float ShieldChance = 0.0f;
	
	if (ShieldSkill < 5.0f)
	{
		// スキル5未満: スキル値 × 6
		ShieldChance = ShieldSkill * 6.0f;
	}
	else
	{
		// スキル5以上: 30 + (スキル値 - 5) × (60/95)
		ShieldChance = 30.0f + (ShieldSkill - 5.0f) * (60.0f / 95.0f);
	}
	
	// 器用さボーナス
	ShieldChance += Talent.Dexterity * 0.3f;
	
	// 最大95%制限
	DerivedStats.ShieldChance = FMath::Clamp(ShieldChance, 0.0f, 95.0f);
	
	// クリティカル率: 5 + (器用 × 0.5) + (スキルレベル × 0.3)
	DerivedStats.CriticalChance = 5.0f + (Talent.Dexterity * 0.5f) + (WeaponSkill * 0.3f);
	
	// 基本ダメージ
	if (EquippedWeapon.IsEmpty())
	{
		// 素手戦闘: 自然武器攻撃力 + (格闘スキル × 0.8) + (力 × 0.7)
		float CombatSkill = GetSkillValue(ESkillType::Combat);
		int32 NaturalAttackPower = 2; // 人間の素手攻撃力
		float NaturalDamage = NaturalAttackPower + (CombatSkill * 0.8f) + (Talent.Strength * 0.7f);
		DerivedStats.BaseDamage = FMath::Max(1, FMath::RoundToInt(NaturalDamage));
	}
	else
	{
		// 武器戦闘: 武器攻撃力 × (1 + スキル/20) + 能力補正
		float WeaponDamage = WeaponAttackPower * (1.0f + (WeaponSkill / 20.0f));
		float AbilityModifier = bIsRangedWeapon ? (Talent.Dexterity * 0.5f) : (Talent.Strength * 0.5f);
		DerivedStats.BaseDamage = FMath::Max(1, FMath::RoundToInt(WeaponDamage + AbilityModifier));
	}
	
	// 防御値: 防具防御力 + (頑丈 × 0.3)
	float ArmorDefense = GetArmorDefense();
	DerivedStats.DefenseValue = FMath::Max(0, FMath::RoundToInt(ArmorDefense + (Talent.Toughness * 0.3f)));
}

void UCharacterStatusComponent::CalculateDisplayStats()
{
	// DPS計算（基本ダメージ × 攻撃速度 × 命中率 × クリティカル補正）
	float HitMultiplier = DerivedStats.HitChance / 100.0f;
	float CritMultiplier = 1.0f + (DerivedStats.CriticalChance / 100.0f); // 簡略化
	DerivedStats.DPS = DerivedStats.BaseDamage * DerivedStats.AttackSpeed * HitMultiplier * CritMultiplier;
	
	UE_LOG(LogTemp, Warning, TEXT("DPS Calc - BaseDamage: %d, AttackSpeed: %.2f, HitChance: %.1f%%, CritChance: %.1f%%"), 
		DerivedStats.BaseDamage, DerivedStats.AttackSpeed, DerivedStats.HitChance, DerivedStats.CriticalChance);
	UE_LOG(LogTemp, Warning, TEXT("DPS Calc - HitMultiplier: %.2f, CritMultiplier: %.2f, Final DPS: %.2f"), 
		HitMultiplier, CritMultiplier, DerivedStats.DPS);
	
	// 総合防御能力（防御値 + 回避率 + 受け流し率の複合）
	float AvoidanceValue = (DerivedStats.DodgeChance + DerivedStats.ParryChance) / 100.0f;
	DerivedStats.TotalDefensePower = DerivedStats.DefenseValue * (1.0f + AvoidanceValue);
	
	UE_LOG(LogTemp, Warning, TEXT("Defense Calc - DefenseValue: %d, DodgeChance: %.1f%%, ParryChance: %.1f%%"), 
		DerivedStats.DefenseValue, DerivedStats.DodgeChance, DerivedStats.ParryChance);
	UE_LOG(LogTemp, Warning, TEXT("Defense Calc - AvoidanceValue: %.2f, Final TotalDefensePower: %.2f"), 
		AvoidanceValue, DerivedStats.TotalDefensePower);
	
	// 戦闘力総合値
	DerivedStats.CombatPower = (DerivedStats.DPS * 0.6f) + (DerivedStats.TotalDefensePower * 0.4f);
	
	// 作業力総合値
	DerivedStats.WorkPower = (DerivedStats.ConstructionPower + DerivedStats.ProductionPower + 
							 DerivedStats.GatheringPower + DerivedStats.CookingPower + 
							 DerivedStats.CraftingPower) / 5.0f;
}

// ヘルパー関数

float UCharacterStatusComponent::GetSkillValue(ESkillType SkillType) const
{
	for (const FSkillTalent& Skill : Talent.Skills)
	{
		if (Skill.SkillType == SkillType)
		{
			return Skill.Value;
		}
	}
	return 1.0f; // デフォルトスキルレベル
}

float UCharacterStatusComponent::GetEquipmentBonus(const FString& StatType) const
{
	// TODO: 装備システム実装後に追加
	return 0.0f;
}

float UCharacterStatusComponent::GetStatusEffectMultiplier(const FString& StatType) const
{
	// TODO: ステータス効果システム実装後に追加
	return 1.0f;
}

FString UCharacterStatusComponent::GetEquippedWeaponId() const
{
	// TODO: 装備システム実装後に正式実装
	// 現在は暫定的にインベントリから武器を検索
	
	AC_IdleCharacter* Character = Cast<AC_IdleCharacter>(GetOwner());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetEquippedWeaponId - No Character found"));
		return TEXT(""); // 素手
	}

	UInventoryComponent* InventoryComp = Character->GetInventoryComponent();
	if (!InventoryComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetEquippedWeaponId - No InventoryComponent found"));
		return TEXT(""); // 素手
	}

	// インベントリ内の最初の武器を使用（暫定）
	TArray<FInventorySlot> AllSlots = InventoryComp->GetAllInventorySlots();
	UE_LOG(LogTemp, Warning, TEXT("GetEquippedWeaponId - Checking %d inventory slots"), AllSlots.Num());
	
	UGameInstance* GameInstance = Character->GetWorld()->GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetEquippedWeaponId - No GameInstance found"));
		return TEXT(""); // 素手
	}

	UItemDataTableManager* ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
	if (!ItemManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetEquippedWeaponId - No ItemManager found"));
		return TEXT(""); // 素手
	}

	for (const FInventorySlot& Slot : AllSlots)
	{
		FItemDataRow ItemData;
		if (ItemManager->GetItemData(Slot.ItemId, ItemData))
		{
			if (ItemData.ItemType == EItemTypeTable::Weapon)
			{
				UE_LOG(LogTemp, Warning, TEXT("GetEquippedWeaponId - Found weapon: %s"), *Slot.ItemId);
				return Slot.ItemId; // 最初に見つかった武器を返す
			}
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("GetEquippedWeaponId - No weapon found, using bare hands"));
	return TEXT(""); // 武器が見つからない場合は素手
}

// 武器・アーマー情報取得のヘルパー関数

float UCharacterStatusComponent::GetWeaponWeight(const FString& WeaponItemId) const
{
	if (WeaponItemId.IsEmpty())
	{
		return 0.5f; // 素手の重量
	}
	
	AC_IdleCharacter* Character = Cast<AC_IdleCharacter>(GetOwner());
	if (!Character)
	{
		return 0.5f;
	}

	UGameInstance* GameInstance = Character->GetWorld()->GetGameInstance();
	if (!GameInstance)
	{
		return 0.5f;
	}

	UItemDataTableManager* ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
	if (!ItemManager)
	{
		return 0.5f;
	}

	FItemDataRow ItemData;
	if (ItemManager->GetItemData(WeaponItemId, ItemData))
	{
		return ItemData.Weight;
	}
	
	return 0.5f;
}

int32 UCharacterStatusComponent::GetWeaponAttackPower(const FString& WeaponItemId) const
{
	if (WeaponItemId.IsEmpty())
	{
		return 3; // 素手攻撃力
	}
	
	AC_IdleCharacter* Character = Cast<AC_IdleCharacter>(GetOwner());
	if (!Character)
	{
		return 3;
	}

	UGameInstance* GameInstance = Character->GetWorld()->GetGameInstance();
	if (!GameInstance)
	{
		return 3;
	}

	UItemDataTableManager* ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
	if (!ItemManager)
	{
		return 3;
	}

	FItemDataRow ItemData;
	if (ItemManager->GetItemData(WeaponItemId, ItemData))
	{
		return ItemData.GetModifiedAttackPower(); // 品質修正済み攻撃力
	}
	
	return 3;
}

ESkillType UCharacterStatusComponent::GetWeaponSkillType(const FString& WeaponItemId) const
{
	if (WeaponItemId.IsEmpty())
	{
		return ESkillType::Combat; // 素手は格闘スキル
	}
	
	// 武器名から対応するスキルタイプを判定
	if (WeaponItemId.Contains(TEXT("sword")) || WeaponItemId.Contains(TEXT("axe")) || WeaponItemId.Contains(TEXT("mace")))
	{
		return ESkillType::OneHandedWeapons;
	}
	else if (WeaponItemId.Contains(TEXT("two_hand")) || WeaponItemId.Contains(TEXT("great")))
	{
		return ESkillType::TwoHandedWeapons;
	}
	else if (WeaponItemId.Contains(TEXT("spear")) || WeaponItemId.Contains(TEXT("halberd")))
	{
		return ESkillType::PolearmWeapons;
	}
	else if (WeaponItemId.Contains(TEXT("bow")))
	{
		return ESkillType::Archery;
	}
	else if (WeaponItemId.Contains(TEXT("gun")))
	{
		return ESkillType::Firearms;
	}
	else if (WeaponItemId.Contains(TEXT("throwing")))
	{
		return ESkillType::Throwing;
	}
	
	return ESkillType::OneHandedWeapons; // デフォルト
}

bool UCharacterStatusComponent::IsRangedWeapon(const FString& WeaponItemId) const
{
	if (WeaponItemId.IsEmpty())
	{
		return false; // 素手は近接
	}
	
	return WeaponItemId.Contains(TEXT("bow")) || 
		   WeaponItemId.Contains(TEXT("gun")) || 
		   WeaponItemId.Contains(TEXT("throwing"));
}

float UCharacterStatusComponent::GetArmorDefense() const
{
	AC_IdleCharacter* Character = Cast<AC_IdleCharacter>(GetOwner());
	if (!Character)
	{
		return 0.0f;
	}

	UInventoryComponent* InventoryComp = Character->GetInventoryComponent();
	if (!InventoryComp)
	{
		return 0.0f;
	}

	float TotalDefense = 0.0f;
	TArray<FInventorySlot> AllSlots = InventoryComp->GetAllInventorySlots();
	
	UGameInstance* GameInstance = Character->GetWorld()->GetGameInstance();
	if (!GameInstance)
	{
		return 0.0f;
	}

	UItemDataTableManager* ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
	if (!ItemManager)
	{
		return 0.0f;
	}

	for (const FInventorySlot& Slot : AllSlots)
	{
		FItemDataRow ItemData;
		if (ItemManager->GetItemData(Slot.ItemId, ItemData))
		{
			if (ItemData.ItemType == EItemTypeTable::Armor)
			{
				TotalDefense += ItemData.GetModifiedDefense(); // 品質修正済み防御力
			}
		}
	}
	
	return TotalDefense;
}