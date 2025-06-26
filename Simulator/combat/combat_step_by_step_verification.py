#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
戦闘計算の各ステップ検証ツール
C++とPythonの計算式を一つずつ比較検証
"""

import random
from dataclasses import dataclass
from typing import Dict
from enum import Enum

class SkillType(Enum):
    OneHandedWeapons = "OneHandedWeapons"
    Combat = "Combat"
    Evasion = "Evasion"
    Parry = "Parry"

@dataclass
class CharacterData:
    name: str
    strength: float
    toughness: float
    dexterity: float
    agility: float
    skills: Dict[SkillType, float]
    weapon_id: str = ""

class CombatStepVerifier:
    def __init__(self):
        # 武器データ（ItemData.csvから）
        self.weapons = {
            "": {"attack_power": 3, "weight": 0.5, "is_ranged": False},
            "short_sword": {"attack_power": 8, "weight": 1.2, "is_ranged": False},
        }
    
    def print_section(self, title: str):
        print(f"\n{'='*60}")
        print(f" {title}")
        print(f"{'='*60}")
    
    def verify_step_by_step(self, attacker: CharacterData, defender: CharacterData):
        """各計算ステップを順番に検証"""
        
        self.print_section("キャラクターデータ")
        print(f"攻撃者: {attacker.name}")
        print(f"  力:{attacker.strength} 頑丈:{attacker.toughness} 器用:{attacker.dexterity} 敏捷:{attacker.agility}")
        print(f"  スキル: {attacker.skills}")
        print(f"  武器: '{attacker.weapon_id}'")
        
        print(f"\n防御者: {defender.name}")
        print(f"  力:{defender.strength} 頑丈:{defender.toughness} 器用:{defender.dexterity} 敏捷:{defender.agility}")
        print(f"  スキル: {defender.skills}")
        print(f"  武器: '{defender.weapon_id}'")
        
        # ステップ1: HP計算
        self.print_section("ステップ1: HP計算")
        self.verify_hp_calculation(attacker)
        self.verify_hp_calculation(defender)
        
        # ステップ2: 武器データ取得
        self.print_section("ステップ2: 武器データ取得")
        self.verify_weapon_data(attacker.weapon_id)
        self.verify_weapon_data(defender.weapon_id)
        
        # ステップ3: スキルレベル取得
        self.print_section("ステップ3: スキルレベル取得")
        self.verify_skill_lookup(attacker)
        self.verify_skill_lookup(defender)
        
        # ステップ4: 基本ダメージ計算
        self.print_section("ステップ4: 基本ダメージ計算")
        self.verify_base_damage_calculation(attacker)
        self.verify_base_damage_calculation(defender)
        
        # ステップ5: 防御値計算
        self.print_section("ステップ5: 防御値計算")
        self.verify_defense_calculation(attacker)
        self.verify_defense_calculation(defender)
        
        # ステップ6: 確率計算
        self.print_section("ステップ6: 確率計算")
        self.verify_probability_calculations(attacker, defender)
        
        # ステップ7: 最終ダメージ計算例
        self.print_section("ステップ7: 最終ダメージ計算例")
        self.verify_final_damage_examples(attacker, defender)
    
    def verify_hp_calculation(self, character: CharacterData):
        """HP計算の検証"""
        print(f"\n{character.name}のHP計算:")
        print("C++実装: FCharacterStatus UCharacterStatusManager::CalculateMaxStatus()")
        print("  計算式: MaxHealth = (Toughness * 3.0f) + (Strength * 1.0f) + 50.0f")
        
        # Python実装
        hp = (character.toughness * 3.0) + (character.strength * 1.0) + 50.0
        
        print(f"  Python: ({character.toughness} * 3.0) + ({character.strength} * 1.0) + 50.0")
        print(f"  結果: {hp}")
    
    def verify_weapon_data(self, weapon_id: str):
        """武器データ取得の検証"""
        print(f"\n武器データ取得: '{weapon_id}'")
        print("C++実装: UCombatCalculator::GetWeaponAttackPower(), GetWeaponWeight()")
        
        if not weapon_id or weapon_id == "unarmed":
            print("  → 素手判定: 攻撃力3, 重量0.5")
            attack_power = 3
            weight = 0.5
        else:
            weapon_data = self.weapons.get(weapon_id, self.weapons[""])
            attack_power = weapon_data["attack_power"]
            weight = weapon_data["weight"]
            print(f"  → 武器データ: 攻撃力{attack_power}, 重量{weight}")
        
        print(f"  Python結果: 攻撃力{attack_power}, 重量{weight}")
    
    def verify_skill_lookup(self, character: CharacterData):
        """スキルレベル取得の検証"""
        print(f"\n{character.name}のスキル取得:")
        print("C++実装: UCombatCalculator::GetSkillLevel(), GetWeaponSkillType()")
        
        weapon_id = character.weapon_id
        if not weapon_id or weapon_id == "unarmed":
            skill_type = SkillType.Combat
            print("  → 素手: Combatスキル使用")
        else:
            skill_type = SkillType.OneHandedWeapons
            print(f"  → {weapon_id}: OneHandedWeaponsスキル使用")
        
        skill_level = character.skills.get(skill_type, 1.0)
        print(f"  Python結果: {skill_type.value} = {skill_level}")
    
    def verify_base_damage_calculation(self, character: CharacterData):
        """基本ダメージ計算の検証"""
        print(f"\n{character.name}の基本ダメージ計算:")
        print("C++実装: UCombatCalculator::CalculateBaseDamage()")
        
        weapon_id = character.weapon_id
        is_unarmed = not weapon_id or weapon_id == "unarmed"
        
        if is_unarmed:
            print("  → 素手戦闘ブランチ")
            print("  計算式: 3 + (CombatSkill * 0.8f) + (Strength * 0.7f)")
            
            combat_skill = character.skills.get(SkillType.Combat, 1.0)
            base_unarmed_damage = 3
            
            unarmed_damage = base_unarmed_damage + (combat_skill * 0.8) + (character.strength * 0.7)
            result = max(1, int(unarmed_damage))
            
            print(f"  Python: {base_unarmed_damage} + ({combat_skill} * 0.8) + ({character.strength} * 0.7)")
            print(f"        = {base_unarmed_damage} + {combat_skill * 0.8} + {character.strength * 0.7}")
            print(f"        = {unarmed_damage}")
            print(f"  結果: {result}")
            
        else:
            print("  → 武器戦闘ブランチ")
            print("  計算式: WeaponDamage + AbilityModifier")
            print("    WeaponDamage = WeaponAttackPower * (1 + (SkillLevel / 20))")
            print("    AbilityModifier = Strength * 0.5 (近接武器)")
            
            skill_level = character.skills.get(SkillType.OneHandedWeapons, 1.0)
            weapon_attack_power = self.weapons.get(weapon_id, self.weapons[""])["attack_power"]
            
            weapon_damage = weapon_attack_power * (1.0 + (skill_level / 20.0))
            ability_modifier = character.strength * 0.5
            result = max(1, int(weapon_damage + ability_modifier))
            
            print(f"  Python: {weapon_attack_power} * (1 + ({skill_level} / 20)) + ({character.strength} * 0.5)")
            print(f"        = {weapon_attack_power} * {1.0 + (skill_level / 20.0)} + {ability_modifier}")
            print(f"        = {weapon_damage} + {ability_modifier}")
            print(f"        = {weapon_damage + ability_modifier}")
            print(f"  結果: {result}")
    
    def verify_defense_calculation(self, character: CharacterData):
        """防御値計算の検証"""
        print(f"\n{character.name}の防御値計算:")
        print("C++実装: UCombatCalculator::CalculateDefenseValue()")
        print("  計算式: ArmorDefense + (Toughness * 0.3f)")
        
        armor_defense = 0.0  # 現在は防具なし
        defense_value = max(0, int(armor_defense + (character.toughness * 0.3)))
        
        print(f"  Python: {armor_defense} + ({character.toughness} * 0.3)")
        print(f"        = {armor_defense} + {character.toughness * 0.3}")
        print(f"  結果: {defense_value}")
    
    def verify_probability_calculations(self, attacker: CharacterData, defender: CharacterData):
        """確率計算の検証"""
        print(f"\n確率計算:")
        print("C++実装: CalculateHitChance, CalculateDodgeChance, etc.")
        
        # 命中率
        weapon_id = attacker.weapon_id
        skill_type = SkillType.Combat if (not weapon_id or weapon_id == "unarmed") else SkillType.OneHandedWeapons
        skill_level = attacker.skills.get(skill_type, 1.0)
        weapon_weight = 0.5 if (not weapon_id or weapon_id == "unarmed") else self.weapons.get(weapon_id, self.weapons[""])["weight"]
        
        base_hit_chance = 50.0 + (skill_level * 2.0) + (attacker.dexterity * 1.5)
        weight_penalty = weapon_weight / (attacker.strength * 0.5)
        hit_chance = max(5.0, base_hit_chance - weight_penalty)
        
        print(f"  命中率計算:")
        print(f"    基本命中率 = 50 + ({skill_level} * 2) + ({attacker.dexterity} * 1.5) = {base_hit_chance}")
        print(f"    重量ペナルティ = {weapon_weight} / ({attacker.strength} * 0.5) = {weight_penalty}")
        print(f"    最終命中率 = {base_hit_chance} - {weight_penalty} = {hit_chance}%")
        
        # 回避率
        evasion_skill = defender.skills.get(SkillType.Evasion, 1.0)
        dodge_chance = max(0.0, 10.0 + (defender.agility * 2.0) + (evasion_skill * 3.0))
        
        print(f"  回避率計算:")
        print(f"    回避率 = 10 + ({defender.agility} * 2) + ({evasion_skill} * 3) = {dodge_chance}%")
    
    def verify_final_damage_examples(self, attacker: CharacterData, defender: CharacterData):
        """最終ダメージ計算例"""
        print(f"\n最終ダメージ計算例:")
        print("C++実装: UCombatCalculator::CalculateFinalDamage()")
        
        # 基本ダメージを再計算
        weapon_id = attacker.weapon_id
        is_unarmed = not weapon_id or weapon_id == "unarmed"
        
        if is_unarmed:
            combat_skill = attacker.skills.get(SkillType.Combat, 1.0)
            base_damage = max(1, int(3 + (combat_skill * 0.8) + (attacker.strength * 0.7)))
        else:
            skill_level = attacker.skills.get(SkillType.OneHandedWeapons, 1.0)
            weapon_attack_power = self.weapons.get(weapon_id, self.weapons[""])["attack_power"]
            weapon_damage = weapon_attack_power * (1.0 + (skill_level / 20.0))
            ability_modifier = attacker.strength * 0.5
            base_damage = max(1, int(weapon_damage + ability_modifier))
        
        # 防御値
        defense_value = max(0, int((defender.toughness * 0.3)))
        
        # 通常ダメージ
        normal_damage = max(1, int(base_damage * (100.0 / (100.0 + defense_value))))
        
        # クリティカルダメージ
        critical_damage = max(1, int(base_damage * 2.0 * (100.0 / (100.0 + defense_value))))
        
        print(f"  基本ダメージ: {base_damage}")
        print(f"  防御値: {defense_value}")
        print(f"  通常ダメージ: {base_damage} * (100 / (100 + {defense_value})) = {normal_damage}")
        print(f"  クリティカルダメージ: {base_damage} * 2 * (100 / (100 + {defense_value})) = {critical_damage}")


def create_realistic_test_characters():
    """実際のゲームログに近いキャラクター作成"""
    
    # 橋本（実際のログから推測される能力値）
    # ダメージ4-21から逆算
    # 素手ダメージ = 3 + (Combat * 0.8) + (Strength * 0.7)
    # 4 = 3 + (Combat * 0.8) + (Strength * 0.7) → Combat * 0.8 + Strength * 0.7 = 1
    # 21 = 3 + (Combat * 0.8) + (Strength * 0.7) → Combat * 0.8 + Strength * 0.7 = 18
    
    # 可能性1: Combat=5, Strength=15 → 5*0.8 + 15*0.7 = 4 + 10.5 = 14.5
    # 可能性2: Combat=10, Strength=10 → 10*0.8 + 10*0.7 = 8 + 7 = 15
    
    hashimoto = CharacterData(
        name="橋本",
        strength=12.0,  # 実際のログから推測
        toughness=18.0,  # HP124.9から逆算: (18*3) + (12*1) + 50 = 116 (近い値)
        dexterity=15.0,
        agility=20.0,
        skills={SkillType.Combat: 8.0, SkillType.Evasion: 6.0},
        weapon_id=""
    )
    
    # ゴブリン（CharacterPresets.csvの値を使用）
    goblin = CharacterData(
        name="ゴブリン", 
        strength=15.0,
        toughness=16.0,
        dexterity=16.0,
        agility=17.0,
        skills={SkillType.OneHandedWeapons: 10.0, SkillType.Evasion: 8.0},
        weapon_id="short_sword"
    )
    
    return hashimoto, goblin


def main():
    print("=== 戦闘計算ステップバイステップ検証 ===")
    print("実際のゲームログに基づくキャラクター設定で検証")
    
    hashimoto, goblin = create_realistic_test_characters()
    verifier = CombatStepVerifier()
    
    print(f"\n実際のログ参考値:")
    print(f"  橋本ダメージ: 4-21 (クリティカル42)")
    print(f"  ゴブリンダメージ: 4 (クリティカル8)")
    print(f"  橋本HP: 124.9")
    
    verifier.verify_step_by_step(hashimoto, goblin)
    
    print(f"\n{'='*60}")
    print(" 検証結果と実際のログとの比較")
    print(f"{'='*60}")
    print("上記の計算結果が実際のゲームログと一致するか確認してください。")
    print("不一致の場合、C++実装またはキャラクターデータに問題があります。")


if __name__ == "__main__":
    main()