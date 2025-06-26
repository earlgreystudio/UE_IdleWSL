#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
スキルベース戦闘バランス調整シミュレーター
武器・防具の必要スキルと実際のスキルレベルに基づくバランス調整
"""

import random
import math
from dataclasses import dataclass
from typing import Dict, List, Tuple
from enum import Enum


@dataclass
class WeaponData:
    """武器データ"""
    name: str
    attack_power: int
    required_skill: int
    weight: float = 1.0


@dataclass
class ArmorData:
    """防具データ"""
    name: str
    defense_power: int
    required_skill: int


@dataclass
class ShieldData:
    """盾データ"""
    name: str
    defense_power: int
    required_skill: int
    block_rate_bonus: float = 0.0  # 盾防御率ボーナス


@dataclass
class CombatStats:
    """戦闘能力値"""
    hit_power: float
    dodge_power: float
    damage: int
    hp: int
    defense: int
    weapon_skill: int = 0
    shield_skill: int = 0
    evasion_skill: int = 0
    parry_skill: int = 0
    # 計算済み防御能力
    dodge_chance: float = 0.0
    parry_chance: float = 0.0
    shield_chance: float = 0.0
    shield_defense: int = 0  # 盾の防御力


class SkillBasedCombatSimulator:
    """スキルベース戦闘シミュレーター"""
    
    def __init__(self):
        self.weapons = self.create_weapon_data()
        self.armors = self.create_armor_data()
        self.shields = self.create_shield_data()
    
    def create_weapon_data(self) -> Dict[str, WeaponData]:
        """武器データ作成 - 攻撃力はスキルレベルと直感的に対応"""
        return {
            # 近接武器
            "unarmed": WeaponData("素手", 2, 0, 0.5),
            "club": WeaponData("棍棒", 5, 5, 1.0),
            "short_sword": WeaponData("ショートソード", 10, 10, 1.2),
            "long_sword": WeaponData("ロングソード", 15, 15, 1.5),
            # 遠距離武器
            "short_bow": WeaponData("短弓", 8, 8, 0.8),
            "long_bow": WeaponData("長弓", 12, 12, 1.0),
            "crossbow": WeaponData("クロスボウ", 15, 15, 1.5),
            "pistol": WeaponData("拳銃", 10, 10, 0.6),
            "rifle": WeaponData("ライフル", 18, 18, 2.0),
        }
    
    def get_natural_weapon_power(self, skill_level: int) -> int:
        """自然武器の攻撃力をスキルレベルの半分程度で計算"""
        return max(2, int(skill_level * 0.4) + 2)  # 0.5→0.4に下げて素手を弱体化
    
    def calculate_dodge_chance(self, agility: float, evasion_skill: int) -> float:
        """回避率計算: 10 + (敏捷 × 2) + (回避スキル × 2) - 調整済み"""
        return 10.0 + (agility * 2.0) + (evasion_skill * 2.0)
    
    def calculate_parry_chance(self, dexterity: float, parry_skill: int) -> float:
        """受け流し率計算: 5 + (器用 × 1.5) + (受け流しスキル × 3)"""
        return 5.0 + (dexterity * 1.5) + (parry_skill * 3.0)
    
    def calculate_shield_chance(self, dexterity: float, shield_skill: int) -> float:
        """
        盾発動率計算: スキル5→30%, スキル100→90%
        器用さも影響
        """
        if shield_skill < 5:
            base_chance = shield_skill * 6.0  # スキル0で0%, スキル5で30%
        else:
            # スキル5で30%, スキル100で90%の線形補間
            base_chance = 30.0 + (shield_skill - 5) * (60.0 / 95.0)
        
        # 器用さボーナス（控えめ）
        dexterity_bonus = dexterity * 0.3
        
        return max(0.0, min(95.0, base_chance + dexterity_bonus))
    
    def calculate_shield_damage_reduction(self, shield_defense: int, shield_skill: int) -> float:
        """
        盾ダメージカット率計算
        防御力5で50%カット、防御力100で98%カット
        盾防御力と盾スキル両方が影響
        """
        if shield_defense == 0 and shield_skill == 0:
            return 1.0  # 完全にカットなし
        
        # 防御値 = 盾防御力 + 盾スキル
        defense_value = shield_defense + shield_skill
        
        # ダメージ倍率計算: 1/(1 + defense_value * 0.5)
        # 最低2%は通すため、最大98%カット
        damage_multiplier = max(0.02, 1.0 / (1.0 + defense_value * 0.5))
        
        return damage_multiplier
    
    def is_ranged_weapon(self, weapon_name: str) -> bool:
        """遠距離武器判定"""
        ranged_weapons = ["short_bow", "long_bow", "crossbow", "pistol", "rifle"]
        return weapon_name in ranged_weapons
    
    def create_armor_data(self) -> Dict[str, ArmorData]:
        """防具データ作成"""
        return {
            "none": ArmorData("防具なし", 0, 0),
            "leather": ArmorData("革鎧", 3, 5),
            "iron": ArmorData("鉄鎧", 6, 10),
            "steel": ArmorData("鋼鎧", 10, 15),
        }
    
    def create_shield_data(self) -> Dict[str, ShieldData]:
        """盾データ作成 - 発動率ボーナスを削除"""
        return {
            "none": ShieldData("盾なし", 0, 0, 0.0),
            "wooden_shield": ShieldData("木の盾", 2, 5, 0.0),
            "iron_shield": ShieldData("鉄の盾", 5, 10, 0.0),
            "steel_shield": ShieldData("鋼の盾", 8, 15, 0.0),
        }
    
    def calculate_hit_chance(self, hit_power: float, dodge_power: float) -> float:
        """
        高速戦闘向け比率ベース命中率計算式
        2:1以上 → 95%、1:1 → 70%、1:2 → 50%
        """
        if hit_power <= 0 or dodge_power <= 0:
            return 70.0
        
        ratio = hit_power / dodge_power
        
        if ratio >= 2.0:
            return 95.0
        elif ratio >= 1.0:
            return 70.0 + (ratio - 1.0) * 25.0
        else:
            return 50.0 + (ratio - 0.5) * 40.0
    
    def calculate_skill_efficiency(self, actual_skill: int, required_skill: int) -> float:
        """
        スキル効率計算
        適正スキル未満: 20%～100%の線形補間
        適正スキル以上: 100%＋（超過分×5%）（上限なし）
        """
        if required_skill == 0:
            return 1.0 + (actual_skill * 0.05)  # 素手もスキルで向上
        
        if actual_skill >= required_skill:
            # 適正スキル以上：100% + 超過分×5%
            excess_skill = actual_skill - required_skill
            return 1.0 + (excess_skill * 0.05)
        else:
            # スキル0で20%、適正スキルで100%の線形補間
            return 0.2 + (actual_skill / required_skill) * 0.8
    
    def create_character(self, talent_level: float, weapon_name: str = "unarmed", 
                        armor_name: str = "none", weapon_skill: int = 0,
                        shield_skill: int = 0, evasion_skill: int = 0, parry_skill: int = 0,
                        shield_name: str = "none") -> CombatStats:
        """キャラクター作成"""
        weapon = self.weapons[weapon_name]
        armor = self.armors[armor_name]
        shield = self.shields[shield_name]
        
        # 基本能力値（タレントレベル±ランダム）
        base_hit = talent_level + random.uniform(-1, 1)
        base_dodge = talent_level + random.uniform(-1, 1)
        
        # 基本ステータス（能力値として使用） 
        agility = talent_level + random.uniform(-1, 1)
        dexterity = talent_level + random.uniform(-1, 1)
        toughness = talent_level + random.uniform(-1, 1)
        
        # スキル効率計算
        weapon_efficiency = self.calculate_skill_efficiency(weapon_skill, weapon.required_skill)
        armor_efficiency = self.calculate_skill_efficiency(weapon_skill, armor.required_skill)  # 簡略化：武器スキルで防具も判定
        
        # 武器攻撃力の決定（自然武器の場合はスキルレベルの半分）
        if weapon_name == "unarmed":
            actual_weapon_power = self.get_natural_weapon_power(weapon_skill)
        else:
            actual_weapon_power = weapon.attack_power
        
        # 武器による命中力・ダメージ補正（スキル効率適用）
        # 遠距離武器は器用さ依存、近接武器は力依存
        is_ranged = self.is_ranged_weapon(weapon_name)
        
        weapon_hit_bonus = actual_weapon_power * 0.8 * weapon_efficiency  # 0.5→0.8に強化
        weapon_damage_bonus = actual_weapon_power * 1.5 * weapon_efficiency  # 1.0→1.5に強化
        
        # 能力値補正の調整
        if is_ranged:
            ability_modifier = dexterity * 0.5  # 遠距離は器用さ
        else:
            ability_modifier = talent_level * 0.5  # 近接は力（talent_level で代用）
        
        # 防具による防御補正（スキル効率適用）
        armor_defense_bonus = armor.defense_power * armor_efficiency
        shield_defense_bonus = shield.defense_power  # 盾は直接防御力に加算
        
        # 最終能力値計算
        hit_power = max(1, base_hit + weapon_hit_bonus)
        dodge_power = max(1, base_dodge + talent_level * 0.5)  # 回避は基本能力値依存
        damage = int(talent_level * 2 + weapon_damage_bonus + ability_modifier + random.uniform(-2, 2))
        hp = int(talent_level * 8 + 50 + random.uniform(-5, 5))
        defense = int(talent_level * 0.5 + armor_defense_bonus + shield_defense_bonus + random.uniform(-1, 1))
        
        # 防御スキル計算
        dodge_chance = self.calculate_dodge_chance(agility, evasion_skill)
        parry_chance = self.calculate_parry_chance(dexterity, parry_skill)
        shield_chance = self.calculate_shield_chance(dexterity, shield_skill)
        
        # 盾装備時の回避率ペナルティ
        if shield_name != "none":
            dodge_chance *= 0.5  # 盾装備時は回避率半減
        
        return CombatStats(hit_power, dodge_power, damage, hp, defense, weapon_skill,
                          shield_skill, evasion_skill, parry_skill,
                          dodge_chance, parry_chance, shield_chance, shield.defense_power)
    
    def single_combat_round(self, attacker: CombatStats, defender: CombatStats) -> int:
        """単一戦闘ラウンド（ダメージを返す）- 防御スキル対応"""
        
        # 1. 基本命中判定（回避スキルも考慮）
        hit_chance = self.calculate_hit_chance(attacker.hit_power, defender.dodge_power)
        
        # 回避スキル追加判定
        if random.uniform(0, 100) < defender.dodge_chance:
            return 0  # 回避成功
            
        if random.uniform(0, 100) >= hit_chance:
            return 0  # 基本命中失敗
        
        # 2. 命中した場合の防御判定
        damage_multiplier = 1.0
        
        # 受け流し判定（80%ダメージカット）
        if random.uniform(0, 100) < defender.parry_chance:
            damage_multiplier *= 0.2  # 80%カット
        
        # 盾防御判定（盾の防御力とスキルに応じてカット率変動）
        if random.uniform(0, 100) < defender.shield_chance:
            shield_reduction = self.calculate_shield_damage_reduction(defender.shield_defense, defender.shield_skill)
            damage_multiplier *= shield_reduction
        
        # 3. 最終ダメージ計算
        base_damage = attacker.damage * (100.0 / (100.0 + defender.defense))
        final_damage = base_damage * damage_multiplier
        
        return max(1, int(final_damage))
    
    def battle_simulation(self, char1: CombatStats, char2: CombatStats) -> bool:
        """戦闘シミュレーション（True: char1勝利）"""
        hp1, hp2 = char1.hp, char2.hp
        max_rounds = 200
        
        for _ in range(max_rounds):
            # char1の攻撃
            damage = self.single_combat_round(char1, char2)
            hp2 -= damage
            if hp2 <= 0:
                return True
                
            # char2の攻撃
            damage = self.single_combat_round(char2, char1)
            hp1 -= damage
            if hp1 <= 0:
                return False
        
        return hp1 > hp2
    
    def comprehensive_combat_test(self):
        """包括的戦闘テスト - 様々な敵・条件でのシミュレーション"""
        print("=== 包括的戦闘バランステスト ===")
        print("スキル効率: 適正以上で100%+超過分×5%、未満は20-100%線形補間")
        print()
        
        # 1. 基本スキル効率テスト
        self.test_skill_efficiency()
        
        # 2. 様々な敵タイプとの戦闘
        self.test_various_enemies()
        
        # 3. 装備格差テスト
        self.test_equipment_disparity()
        
        # 4. レベル差戦闘テスト
        self.test_level_differences()
    
    def test_skill_efficiency(self):
        """スキル効率の詳細テスト"""
        print("【1. スキル効率テスト】")
        print("武器: ショートソード (攻撃力12, 必要スキル10)")
        print("スキル | 効率   | 命中力 | ダメージ | 敵Lv10勝率")
        print("-" * 50)
        
        for skill in range(0, 31, 3):
            efficiency = self.calculate_skill_efficiency(skill, 10)
            
            wins = 0
            total_hit_power = 0
            total_damage = 0
            simulations = 100
            
            for _ in range(simulations):
                player = self.create_character(10, "short_sword", "leather", skill)
                enemy = self.create_character(10, "unarmed", "none", 10)
                
                total_hit_power += player.hit_power
                total_damage += player.damage
                
                if self.battle_simulation(player, enemy):
                    wins += 1
            
            win_rate = (wins / simulations) * 100
            avg_hit_power = total_hit_power / simulations
            avg_damage = total_damage / simulations
            
            print(f"{skill:4d}   | {efficiency:5.2f} | {avg_hit_power:6.1f} | {avg_damage:6.1f}   | {win_rate:7.1f}%")
        print()
    
    def test_various_enemies(self):
        """様々な敵タイプとの戦闘テスト"""
        print("【2. 様々な敵タイプ戦闘テスト】")
        print("🛡️ プレイヤー固定条件:")
        print("   タレント10, ショートソード(攻撃力12), 革鎧, 片手武器スキル10")
        print()
        
        # 敵の設定パターン
        enemy_types = [
            ("格下動物", 6, "unarmed", "none", 3, "弱い動物・昆虫"),
            ("同等動物", 10, "unarmed", "none", 10, "牙・爪を持つ同等の動物"),
            ("格上動物", 14, "unarmed", "none", 14, "大型猛獣・強力な牙爪"),
            ("未熟盗賊", 8, "club", "none", 5, "棍棒を持った駆け出し盗賊"),
            ("熟練盗賊", 10, "short_sword", "leather", 10, "同等装備の熟練盗賊"),
            ("精鋭兵士", 12, "long_sword", "iron", 15, "上位装備の精鋭兵士"),
        ]
        
        for enemy_name, talent, weapon, armor, skill, description in enemy_types:
            wins = 0
            total_player_hit = 0
            total_enemy_hit = 0
            total_player_damage = 0
            total_enemy_damage = 0
            simulations = 100
            
            for _ in range(simulations):
                player = self.create_character(10, "short_sword", "leather", 10)
                enemy = self.create_character(talent, weapon, armor, skill)
                
                total_player_hit += self.calculate_hit_chance(player.hit_power, enemy.dodge_power)
                total_enemy_hit += self.calculate_hit_chance(enemy.hit_power, player.dodge_power)
                total_player_damage += player.damage
                total_enemy_damage += enemy.damage
                
                if self.battle_simulation(player, enemy):
                    wins += 1
            
            win_rate = (wins / simulations) * 100
            avg_player_hit = total_player_hit / simulations
            avg_enemy_hit = total_enemy_hit / simulations
            hit_diff = avg_player_hit - avg_enemy_hit
            avg_player_damage = total_player_damage / simulations
            avg_enemy_damage = total_enemy_damage / simulations
            
            # 自然武器攻撃力計算
            if weapon == "unarmed":
                natural_power = self.get_natural_weapon_power(skill)
                weapon_display = f"自然武器(攻撃力{natural_power})"
            else:
                weapon_display = weapon
            
            print(f"🎯 **{enemy_name}** - 勝率: **{win_rate:.0f}%**")
            print(f"   能力: タレント{talent}, {weapon_display}, {armor}防具, スキル{skill}")
            print(f"   {description}")
            print(f"   命中率差: {hit_diff:+.1f}% (プレイヤー{avg_player_hit:.0f}% vs 敵{avg_enemy_hit:.0f}%)")
            print(f"   ダメージ差: プレイヤー{avg_player_damage:.0f} vs 敵{avg_enemy_damage:.0f}")
            
            # バランス評価
            if weapon == "unarmed" and talent >= 10 and win_rate > 60:
                print(f"   ⚠️  **バランス警告**: 同等レベル自然武器敵に{win_rate:.0f}%勝率は高すぎ")
            elif 40 <= win_rate <= 60:
                print(f"   ✅ **バランス良好**: 接戦の好勝負")
            elif win_rate >= 80:
                print(f"   💪 **圧倒的有利**: 装備・能力差が顕著")
            elif win_rate <= 20:
                print(f"   😰 **厳しい戦い**: 装備・能力不足")
            print()
    
    def test_equipment_disparity(self):
        """装備格差テスト"""
        print("【3. 装備格差テスト】")
        print("プレイヤーvs敵 (両者タレント10, スキル10)")
        print()
        
        # 装備組み合わせ
        equipment_tests = [
            ("素手 vs 素手", "unarmed", "none", "unarmed", "none"),
            ("棍棒 vs 素手", "club", "leather", "unarmed", "none"),
            ("短剣 vs 棍棒", "short_sword", "leather", "club", "leather"),
            ("長剣 vs 短剣", "long_sword", "iron", "short_sword", "leather"),
            ("長剣 vs 素手", "long_sword", "steel", "unarmed", "none"),
        ]
        
        print("戦闘組み合わせ         | プレイヤー武器  | プレイヤー防具 | 敵武器     | 敵防具   | 勝率")
        print("-" * 80)
        
        for test_name, p_weapon, p_armor, e_weapon, e_armor in equipment_tests:
            wins = 0
            simulations = 100
            
            for _ in range(simulations):
                player = self.create_character(10, p_weapon, p_armor, 10)
                enemy = self.create_character(10, e_weapon, e_armor, 10)
                
                if self.battle_simulation(player, enemy):
                    wins += 1
            
            win_rate = (wins / simulations) * 100
            print(f"{test_name:20s} | {p_weapon:12s}  | {p_armor:10s} | {e_weapon:8s}   | {e_armor:6s}   | {win_rate:5.1f}%")
        print()
    
    def test_level_differences(self):
        """レベル差戦闘テスト"""
        print("【4. レベル差戦闘テスト】")
        print("プレイヤー: ショートソード, スキル10")
        print("敵: 素手, スキル=タレント")
        print()
        
        player_talents = [5, 10, 15]
        enemy_talent_diffs = [-3, -2, -1, 0, +1, +2, +3, +4, +5]
        
        print("プレイヤー\\敵タレント差", end="")
        for diff in enemy_talent_diffs:
            print(f" | {diff:+3d}", end="")
        print()
        print("-" * (25 + len(enemy_talent_diffs) * 6))
        
        for p_talent in player_talents:
            print(f"タレント{p_talent:2d}          ", end="")
            
            for talent_diff in enemy_talent_diffs:
                e_talent = p_talent + talent_diff
                if e_talent < 1:
                    print(" |  -- ", end="")
                    continue
                
                wins = 0
                simulations = 50  # 計算量削減
                
                for _ in range(simulations):
                    player = self.create_character(p_talent, "short_sword", "leather", 10)
                    enemy = self.create_character(e_talent, "unarmed", "none", e_talent)
                    
                    if self.battle_simulation(player, enemy):
                        wins += 1
                
                win_rate = (wins / simulations) * 100
                print(f" | {win_rate:3.0f}", end="")
            print()
        print()
    
    def detailed_skill_analysis(self):
        """詳細スキル分析"""
        print("\\n=== 詳細スキル効果分析 ===")
        
        talent = 10
        weapon_name = "short_sword"
        weapon = self.weapons[weapon_name]
        
        print(f"分析対象: タレント{talent}, {weapon.name} (必要スキル{weapon.required_skill})")
        print()
        print("スキル | 効率   | 実際命中力 | 実際ダメージ | 防御力")
        print("-" * 50)
        
        for skill in range(0, 21, 2):
            player = self.create_character(talent, weapon_name, "leather", skill)
            efficiency = self.calculate_skill_efficiency(skill, weapon.required_skill)
            
            print(f"{skill:4d}   | {efficiency:5.1f} | {player.hit_power:8.1f} | {player.damage:10.1f} | {player.defense:6.1f}")


    def player_weapon_skill_comparison(self):
        """プレイヤーの武器・スキル組み合わせ比較"""
        print("【プレイヤー武器・スキル組み合わせ比較】")
        print("🎯 固定条件: タレント10, 革鎧")
        print()
        
        # プレイヤー設定パターン
        player_configs = [
            ("素手+格闘10", "unarmed", 10, "格闘スキル"),
            ("ショートソード+片手0", "short_sword", 0, "武器習得前"),
            ("ショートソード+片手10", "short_sword", 10, "適正スキル"),
            ("ショートソード+片手20", "short_sword", 20, "熟練者"),
        ]
        
        # 対戦相手（弱い敵も追加）
        enemies = [
            ("動物1", 1, "unarmed", "none", 1),
            ("動物5", 5, "unarmed", "none", 5),
            ("動物6", 6, "unarmed", "none", 3),
            ("動物10", 10, "unarmed", "none", 10),
            ("盗賊5", 8, "club", "none", 5),
            ("盗賊10", 10, "short_sword", "leather", 10),
        ]
        
        for player_name, weapon, skill, description in player_configs:
            print(f"🛡️ **{player_name}** ({description})")
            
            # プレイヤー能力値表示
            player_sample = self.create_character(10, weapon, "leather", skill)
            if weapon == "unarmed":
                weapon_power = self.get_natural_weapon_power(skill)
                weapon_display = f"自然武器(攻撃力{weapon_power})"
            else:
                weapon_power = self.weapons[weapon].attack_power
                weapon_display = f"{weapon}(攻撃力{weapon_power})"
            
            efficiency = self.calculate_skill_efficiency(skill, self.weapons[weapon].required_skill if weapon != "unarmed" else 0)
            
            print(f"   武器: {weapon_display}, スキル効率: {efficiency:.2f}")
            print(f"   能力値: 命中力{player_sample.hit_power:.0f}, ダメージ{player_sample.damage:.0f}")
            print()
            
            # 各敵との戦闘テスト
            for enemy_name, e_talent, e_weapon, e_armor, e_skill in enemies:
                wins = 0
                total_player_hit = 0
                total_enemy_hit = 0
                simulations = 100
                
                for _ in range(simulations):
                    player = self.create_character(10, weapon, "leather", skill)
                    enemy = self.create_character(e_talent, e_weapon, e_armor, e_skill)
                    
                    total_player_hit += self.calculate_hit_chance(player.hit_power, enemy.dodge_power)
                    total_enemy_hit += self.calculate_hit_chance(enemy.hit_power, player.dodge_power)
                    
                    if self.battle_simulation(player, enemy):
                        wins += 1
                
                win_rate = (wins / simulations) * 100
                avg_player_hit = total_player_hit / simulations
                avg_enemy_hit = total_enemy_hit / simulations
                hit_diff = avg_player_hit - avg_enemy_hit
                
                # 自然武器表示
                if e_weapon == "unarmed":
                    e_weapon_power = self.get_natural_weapon_power(e_skill)
                    enemy_weapon_display = f"自然武器{e_weapon_power}"
                else:
                    enemy_weapon_display = e_weapon
                
                print(f"   vs {enemy_name}: **勝率{win_rate:.0f}%** (命中率差{hit_diff:+.0f}%)")
                
                # バランス評価
                if 45 <= win_rate <= 55:
                    balance_emoji = "⚖️"
                elif win_rate >= 80:
                    balance_emoji = "💪"
                elif win_rate <= 20:
                    balance_emoji = "😰"
                else:
                    balance_emoji = "📊"
                
                print(f"      {balance_emoji} タレント{e_talent}, {enemy_weapon_display}, スキル{e_skill}")
            
            print()
        
        # スキル効果まとめ
        print("📈 **スキル効果まとめ**")
        print()
        
        # ショートソードでのスキル効果
        print("ショートソード使用時のスキル別勝率 (vs動物10):")
        skills_to_test = [0, 10, 20]
        enemy_target = ("動物10", 10, "unarmed", "none", 10)  # 基準敵
        
        for skill in skills_to_test:
            wins = 0
            simulations = 100
            
            for _ in range(simulations):
                player = self.create_character(10, "short_sword", "leather", skill)
                enemy = self.create_character(enemy_target[1], enemy_target[2], enemy_target[3], enemy_target[4])
                
                if self.battle_simulation(player, enemy):
                    wins += 1
            
            win_rate = (wins / simulations) * 100
            efficiency = self.calculate_skill_efficiency(skill, 10)
            
            print(f"   スキル{skill:2d}: 勝率{win_rate:3.0f}% (効率{efficiency:.2f}x)")
    
    def defensive_skills_balance_test(self):
        """防御スキルバランステスト - 各防御スキルの効果測定"""
        print("=== 防御スキルバランステスト ===")
        print("🛡️ ベースキャラクター: ショートソード+片手武器10, タレント10")
        print("🎯 目標: 各適正スキルで勝率+20%程度の向上")
        print()
        
        # ベースライン設定
        base_talent = 10
        base_weapon = "short_sword"
        base_armor = "leather"
        base_weapon_skill = 10
        
        # 敵の設定
        enemy_configs = [
            ("動物5", 5, "unarmed", "none", 5, 5),    # (名前, タレント, 武器, 防具, 武器スキル, 回避スキル)
            ("動物10", 10, "unarmed", "none", 10, 10),
            ("動物15", 15, "unarmed", "none", 15, 15),
        ]
        
        # 防御スキルバリエーション (スキル名, 盾スキル, 回避スキル, 受け流しスキル, 盾装備)
        defense_variations = [
            ("ベースライン", 0, 0, 0, "none"),
            ("木盾+盾スキル0", 0, 0, 0, "wooden_shield"),
            ("木盾+盾スキル10", 10, 0, 0, "wooden_shield"),
            ("鉄盾+盾スキル10", 10, 0, 0, "iron_shield"),
            ("回避スキル0", 0, 0, 0, "none"),
            ("回避スキル10", 0, 10, 0, "none"),
            ("受け流しスキル0", 0, 0, 0, "none"), 
            ("受け流しスキル10", 0, 0, 10, "none"),
        ]
        
        # 各敵に対する防御スキル効果テスト
        for enemy_name, e_talent, e_weapon, e_armor, e_weapon_skill, e_evasion_skill in enemy_configs:
            print(f"【vs {enemy_name}】 (タレント{e_talent}, 回避{e_evasion_skill})")
            print()
            
            baseline_win_rate = None
            
            for var_name, shield_skill, evasion_skill, parry_skill, shield_name in defense_variations:
                wins = 0
                total_damage_taken = 0
                total_damage_dealt = 0
                simulations = 200
                
                for _ in range(simulations):
                    # プレイヤー作成
                    player = self.create_character(
                        base_talent, base_weapon, base_armor, base_weapon_skill,
                        shield_skill, evasion_skill, parry_skill, shield_name
                    )
                    
                    # 敵作成（回避スキルのみ持つ動物）
                    enemy = self.create_character(
                        e_talent, e_weapon, e_armor, e_weapon_skill,
                        0, e_evasion_skill, 0  # 盾0, 回避あり, 受け流し0
                    )
                    
                    # 戦闘前の能力チェック（初回のみ）
                    if _ == 0:
                        if player.shield_defense > 0:
                            shield_defense_total = player.shield_defense + player.shield_skill
                            shield_cut_rate = (1.0 - self.calculate_shield_damage_reduction(player.shield_defense, player.shield_skill)) * 100
                            shield_info = f"盾{player.shield_chance:.0f}%({shield_cut_rate:.0f}%カット)"
                        else:
                            shield_info = f"盾{player.shield_chance:.0f}%"
                        player_defense_info = f"回避{player.dodge_chance:.0f}% 受け流し{player.parry_chance:.0f}% {shield_info}"
                        enemy_defense_info = f"回避{enemy.dodge_chance:.0f}%"
                    
                    # 戦闘シミュレーション
                    player_hp_start = player.hp
                    if self.battle_simulation(player, enemy):
                        wins += 1
                        # 勝利時のダメージ計算は省略
                    
                win_rate = (wins / simulations) * 100
                
                # ベースライン設定
                if var_name == "ベースライン":
                    baseline_win_rate = win_rate
                
                # 改善率計算
                if baseline_win_rate and var_name != "ベースライン":
                    improvement = win_rate - baseline_win_rate
                    improvement_ratio = win_rate / baseline_win_rate if baseline_win_rate > 0 else 1.0
                    
                    # バランス評価
                    if var_name.endswith("10"):  # 適正スキル
                        if improvement >= 15:  # 15%以上向上
                            balance_status = "✅ 良好"
                        elif improvement >= 10:
                            balance_status = "📊 やや効果的"  
                        elif improvement >= 5:
                            balance_status = "⚠️  効果不足"
                        else:
                            balance_status = "❌ 効果なし"
                        
                        improvement_text = f"({improvement:+.0f}%, {improvement_ratio:.2f}x) {balance_status}"
                    else:
                        improvement_text = f"({improvement:+.0f}%)"
                else:
                    improvement_text = "(基準)"
                
                print(f"   {var_name:12s}: **勝率{win_rate:3.0f}%** {improvement_text}")
                if var_name == "ベースライン":
                    print(f"      防御: {player_defense_info}")
                    print(f"      敵防御: {enemy_defense_info}")
            
            print()
        
        # 防御スキル効果まとめ
        print("📊 **新・防御スキル効果まとめ**")
        print("目標: より現実的で段階的な盾システム")
        print("- 回避スキル: 回避率向上で攻撃回避 (係数×2)")
        print("- 受け流しスキル: 受け流し率向上で80%ダメージカット") 
        print("- 盾スキル: 器用さ+スキルで発動率、防御力+スキルでカット率")
        print("  - 発動率: スキル5→30%, スキル100→90% (器用さも影響)")
        print("  - カット率: 防御値5→50%, 防御値100→98% (防御力+スキル)")
        print("  - 木盾(防御2)+スキル10 = 防御値12 → 約86%カット")
        print("  - 鉄盾(防御5)+スキル10 = 防御値15 → 約89%カット") 
        print("  - 盾装備時は回避率が半減するトレードオフあり")
        print("複数防御スキルの重複効果も検証可能")
        print()
    
    def ranged_weapon_balance_test(self):
        """遠距離武器バランステスト"""
        print("=== 遠距離武器バランステスト ===")
        print("🏹 現在の遠距離武器仕様でのバランス検証")
        print("📊 近接武器との比較、遠距離武器間のバランス確認")
        print()
        
        # テスト用キャラクター設定
        base_talent = 10
        base_armor = "leather"
        skill_level = 10
        
        # 武器カテゴリ
        weapon_categories = [
            # (武器名, カテゴリ, 想定スキル)
            ("short_sword", "近接", "片手武器"),
            ("long_sword", "近接", "両手武器"),
            ("short_bow", "遠距離", "弓"),
            ("long_bow", "遠距離", "弓"),
            ("crossbow", "遠距離", "弓"),
            ("pistol", "遠距離", "射撃"),
            ("rifle", "遠距離", "射撃"),
        ]
        
        # 敵の設定
        enemy_configs = [
            ("動物5", 5, "unarmed", "none", 5, 5),
            ("動物10", 10, "unarmed", "none", 10, 10),
            ("盗賊10", 10, "short_sword", "leather", 10, 10),
        ]
        
        print("【武器性能比較】")
        print("武器名        | 攻撃力 | 重量 | vs動物5 | vs動物10 | vs盗賊10 | 特徴")
        print("-" * 75)
        
        for weapon_name, category, skill_type in weapon_categories:
            weapon = self.weapons[weapon_name]
            
            # 各敵に対する勝率テスト
            win_rates = []
            
            for enemy_name, e_talent, e_weapon, e_armor, e_weapon_skill, e_evasion_skill in enemy_configs:
                wins = 0
                simulations = 100
                
                for _ in range(simulations):
                    # プレイヤー作成
                    player = self.create_character(
                        base_talent, weapon_name, base_armor, skill_level
                    )
                    
                    # 敵作成
                    enemy = self.create_character(
                        e_talent, e_weapon, e_armor, e_weapon_skill,
                        0, e_evasion_skill, 0
                    )
                    
                    if self.battle_simulation(player, enemy):
                        wins += 1
                
                win_rate = (wins / simulations) * 100
                win_rates.append(win_rate)
            
            # 武器特徴分析
            is_ranged = self.is_ranged_weapon(weapon_name)
            features = []
            if is_ranged:
                features.append("遠距離")
                if weapon.weight < 1.0:
                    features.append("軽量")
                elif weapon.weight > 1.5:
                    features.append("重量")
            else:
                features.append("近接")
                
            feature_text = "+".join(features)
            
            print(f"{weapon.name:10s} | {weapon.attack_power:4d}   | {weapon.weight:3.1f}  | {win_rates[0]:4.0f}%  | {win_rates[1]:5.0f}%  | {win_rates[2]:5.0f}%  | {feature_text}")
        
        print()
        
        # 遠距離武器の詳細分析
        print("【遠距離武器詳細分析】")
        ranged_weapons = ["short_bow", "long_bow", "crossbow", "pistol", "rifle"]
        
        for weapon_name in ranged_weapons:
            weapon = self.weapons[weapon_name]
            print(f"\n🏹 **{weapon.name}** (攻撃力{weapon.attack_power}, 重量{weapon.weight})")
            
            # vs 動物10での詳細分析
            total_damage = 0
            total_hit_power = 0
            simulations = 50
            
            for _ in range(simulations):
                player = self.create_character(base_talent, weapon_name, base_armor, skill_level)
                enemy = self.create_character(10, "unarmed", "none", 10, 0, 10, 0)
                
                total_damage += player.damage
                total_hit_power += player.hit_power
            
            avg_damage = total_damage / simulations
            avg_hit_power = total_hit_power / simulations
            
            print(f"   平均ダメージ: {avg_damage:.1f}")
            print(f"   平均命中力: {avg_hit_power:.1f}")
            
            # バランス評価
            近接比較 = self.weapons["short_sword"]
            damage_ratio = avg_damage / (base_talent * 2 + 近接比較.attack_power * 1.5 + base_talent * 0.5)
            
            if damage_ratio > 1.2:
                balance_status = "🟡 強い"
            elif damage_ratio > 0.8:
                balance_status = "🟢 バランス良好"
            else:
                balance_status = "🔴 弱い"
                
            print(f"   バランス評価: {balance_status} (近接比 {damage_ratio:.2f}x)")
        
        print()
        print("📊 **遠距離武器バランス分析まとめ**")
        print("- 現在の仕様: 器用さ依存、重量ペナルティなし")
        print("- 近接武器との比較でバランスチェック")
        print("- 弓系 vs 射撃系の性能差確認")
        print("- 重量と攻撃力のトレードオフ評価")
        print()


def main():
    """メイン関数"""
    simulator = SkillBasedCombatSimulator()
    
    # 遠距離武器バランステスト
    simulator.ranged_weapon_balance_test()


if __name__ == "__main__":
    main()