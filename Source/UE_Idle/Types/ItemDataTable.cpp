#include "ItemDataTable.h"

int32 FItemDataRow::GetModifiedAttackPower() const
{
    return FMath::RoundToInt(AttackPower * GetQualityModifier(Quality));
}

int32 FItemDataRow::GetModifiedDefense() const
{
    return FMath::RoundToInt(Defense * GetQualityModifier(Quality));
}

int32 FItemDataRow::GetModifiedDurability() const
{
    return FMath::RoundToInt(MaxDurability * GetQualityModifier(Quality));
}

int32 FItemDataRow::GetModifiedValue() const
{
    return FMath::RoundToInt(BaseValue * GetQualityModifier(Quality));
}

float FItemDataRow::GetQualityModifier(EItemQualityTable Quality)
{
    switch (Quality)
    {
    case EItemQualityTable::Poor:        return 0.7f;   // -30%
    case EItemQualityTable::Common:      return 1.0f;   // 基準値
    case EItemQualityTable::Good:        return 1.3f;   // +30%
    case EItemQualityTable::Masterwork:  return 1.6f;   // +60%
    case EItemQualityTable::Legendary:   return 2.0f;   // +100%
    default:                             return 1.0f;
    }
}

int32 FItemDataRow::GetRealWorldValue() const
{
    float Multiplier = GetCategoryRealWorldMultiplier(TradeCategory);
    return FMath::RoundToInt(BaseValue * Multiplier);
}

int32 FItemDataRow::GetOtherWorldValue() const
{
    float Multiplier = GetCategoryOtherWorldMultiplier(TradeCategory);
    return FMath::RoundToInt(BaseValue * Multiplier);
}

float FItemDataRow::GetCategoryRealWorldMultiplier(ETradeCategoryTable Category)
{
    switch (Category)
    {
    case ETradeCategoryTable::MeleeWeapons:      return 0.1f;   // 現世では実用性なし
    case ETradeCategoryTable::ModernWeapons:     return 1.0f;   // 需要あり
    case ETradeCategoryTable::Gems:              return 1.0f;   // 装飾品価値
    case ETradeCategoryTable::Antiques:          return 1.2f;   // コレクター需要
    case ETradeCategoryTable::Electronics:       return 1.0f;   // 通常価格
    case ETradeCategoryTable::ModernGoods:       return 0.8f;   // 中古品扱い
    case ETradeCategoryTable::Food:              return 1.0f;   // 普通
    case ETradeCategoryTable::MagicMaterials:    return 0.1f;   // オカルト扱い
    case ETradeCategoryTable::MonsterMaterials:  return 2.0f;   // 研究価値
    case ETradeCategoryTable::Medicine:          return 1.0f;   // 医療用
    case ETradeCategoryTable::CommonMaterials:   return 0.9f;   // 一般的
    case ETradeCategoryTable::Luxury:            return 1.1f;   // 贅沢品
    default:                                     return 1.0f;
    }
}

float FItemDataRow::GetCategoryOtherWorldMultiplier(ETradeCategoryTable Category)
{
    switch (Category)
    {
    case ETradeCategoryTable::MeleeWeapons:      return 1.0f;   // 実戦で使用
    case ETradeCategoryTable::ModernWeapons:     return 0.1f;   // 魔法に劣る
    case ETradeCategoryTable::Gems:              return 1.0f;   // 魔法触媒
    case ETradeCategoryTable::Antiques:          return 0.2f;   // 文化的価値なし
    case ETradeCategoryTable::Electronics:       return 0.05f;  // 電気なし
    case ETradeCategoryTable::ModernGoods:       return 1.5f;   // 珍しい技術
    case ETradeCategoryTable::Food:              return 2.0f;   // 未知の味
    case ETradeCategoryTable::MagicMaterials:    return 1.5f;   // 魔法に必須
    case ETradeCategoryTable::MonsterMaterials:  return 1.0f;   // 普通の素材
    case ETradeCategoryTable::Medicine:          return 1.8f;   // 高度な治療
    case ETradeCategoryTable::CommonMaterials:   return 1.0f;   // 標準価格
    case ETradeCategoryTable::Luxury:            return 0.5f;   // 実用性重視
    default:                                     return 1.0f;
    }
}