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