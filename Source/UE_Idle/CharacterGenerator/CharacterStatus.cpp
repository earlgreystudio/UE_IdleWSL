#include "CharacterStatus.h"
#include "Engine/Engine.h"

FCharacterStatus UCharacterStatusManager::CalculateMaxStatus(const FCharacterTalent& Talent)
{
    FCharacterStatus Status;
    
    // 体力計算: (頑丈 × 3) + (力 × 1) + 50
    Status.MaxHealth = (Talent.Toughness * 3.0f) + (Talent.Strength * 1.0f) + 50.0f;
    
    // スタミナ計算: (頑丈 × 2) + (敏捷 × 2) + 30
    Status.MaxStamina = (Talent.Toughness * 2.0f) + (Talent.Agility * 2.0f) + 30.0f;
    
    // メンタル計算: (精神力 × 4) + (賢さ × 1) + 20
    Status.MaxMental = (Talent.Willpower * 4.0f) + (Talent.Intelligence * 1.0f) + 20.0f;
    
    // 積載量計算: (力 × 1.5) + (頑丈 × 1.0) + (敏捷 × 0.5) + 20
    Status.CarryingCapacity = (Talent.Strength * 1.5f) + (Talent.Toughness * 1.0f) + (Talent.Agility * 0.5f) + 20.0f;
    
    // 現在値を最大値に設定（満タン状態で開始）
    Status.CurrentHealth = Status.MaxHealth;
    Status.CurrentStamina = Status.MaxStamina;
    Status.CurrentMental = Status.MaxMental;
    
    return Status;
}