#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
完全バランステスト用戦闘シミュレーター
武器防具バリエーション + 様々な強さの敵でテスト
"""

import random
import math
from dataclasses import dataclass
from typing import List, Dict, Tuple
from enum import Enum


class SkillType(Enum):
    OneHandedWeapons = "OneHandedWeapons"
    TwoHandedWeapons = "TwoHandedWeapons"
    Combat = "Combat"
    Evasion = "Evasion"
    Parry = "Parry"


class ClubType(Enum):
    Baseball = "Baseball"
    Kendo = "Kendo"
    None_ = "None"


@dataclass
class CharacterStats:
    name: str
    strength: float
    toughness: float
    intelligence: float
    dexterity: float
    agility: float
    willpower: float
    skills: Dict[SkillType, float]
    weapon_id: str = ""
    armor_defense: float = 0.0  # 防具防御力
    
    def calculate_health(self) -> float:
        """HP計算: (頑丈 × 3) + (力 × 1) + 50"""
        return (self.toughness * 3.0) + (self.strength * 1.0) + 50.0


@dataclass
class WeaponData:
    name: str
    attack_power: int
    weight: float
    skill_type: SkillType
    is_ranged: bool = False


@dataclass
class CombatResult:
    hit: bool = False
    dodged: bool = False
    parried: bool = False
    critical: bool = False
    base_damage: int = 0
    final_damage: int = 0


class CombatSimulator:
    def __init__(self):
        # 武器データベース（拡張版）
        self.weapons = {
            "": WeaponData("素手", 3, 0.5, SkillType.Combat),
            "unarmed": WeaponData("素手", 3, 0.5, SkillType.Combat),
            
            # 片手武器
            "short_sword": WeaponData("ショートソード", 8, 1.2, SkillType.OneHandedWeapons),
            "iron_sword": WeaponData("鉄の剣", 10, 1.5, SkillType.OneHandedWeapons),
            "steel_sword": WeaponData("鋼の剣", 15, 1.8, SkillType.OneHandedWeapons),
            "magic_sword": WeaponData("魔法の剣", 20, 1.0, SkillType.OneHandedWeapons),
            
            # 両手武器
            "great_sword": WeaponData("大剣", 18, 4.0, SkillType.TwoHandedWeapons),
            "war_hammer": WeaponData("ウォーハンマー", 22, 5.0, SkillType.TwoHandedWeapons),
            
            # 遠距離武器
            "short_bow": WeaponData("ショートボウ", 12, 0.8, SkillType.OneHandedWeapons, True),
            "long_bow": WeaponData("ロングボウ", 16, 1.2, SkillType.OneHandedWeapons, True),
        }
    
    def get_skill_level(self, character: CharacterStats, skill_type: SkillType) -> float:
        return character.skills.get(skill_type, 1.0)
    
    def get_weapon_skill_type(self, weapon_id: str) -> SkillType:
        if not weapon_id or weapon_id == "unarmed":
            return SkillType.Combat
        return self.weapons.get(weapon_id, self.weapons[""]).skill_type
    
    def get_weapon_attack_power(self, weapon_id: str) -> int:
        if not weapon_id or weapon_id == "unarmed":
            return 3
        return self.weapons.get(weapon_id, self.weapons[""]).attack_power
    
    def get_weapon_weight(self, weapon_id: str) -> float:
        if not weapon_id or weapon_id == "unarmed":
            return 0.5
        return self.weapons.get(weapon_id, self.weapons[""]).weight
    
    def is_ranged_weapon(self, weapon_id: str) -> bool:
        if not weapon_id or weapon_id == "unarmed":
            return False
        return self.weapons.get(weapon_id, self.weapons[""]).is_ranged
    
    def calculate_hit_chance(self, attacker: CharacterStats, weapon_id: str) -> float:
        """命中率計算"""
        weapon_skill = self.get_weapon_skill_type(weapon_id)
        skill_level = self.get_skill_level(attacker, weapon_skill)
        weapon_weight = self.get_weapon_weight(weapon_id)
        
        base_hit_chance = 50.0 + (skill_level * 2.0) + (attacker.dexterity * 1.5)
        weight_penalty = weapon_weight / (attacker.strength * 0.5)
        
        return max(5.0, base_hit_chance - weight_penalty)
    
    def calculate_dodge_chance(self, defender: CharacterStats) -> float:
        """回避率計算"""
        evasion_skill = self.get_skill_level(defender, SkillType.Evasion)
        base_dodge_chance = 10.0 + (defender.agility * 2.0) + (evasion_skill * 3.0)
        return max(0.0, base_dodge_chance)
    
    def calculate_parry_chance(self, defender: CharacterStats) -> float:
        """受け流し率計算"""
        parry_skill = self.get_skill_level(defender, SkillType.Parry)
        base_parry_chance = 5.0 + (defender.dexterity * 1.5) + (parry_skill * 3.0)
        return max(0.0, base_parry_chance)
    
    def calculate_critical_chance(self, attacker: CharacterStats, weapon_id: str) -> float:
        """クリティカル率計算"""
        weapon_skill = self.get_weapon_skill_type(weapon_id)
        skill_level = self.get_skill_level(attacker, weapon_skill)
        return 5.0 + (attacker.dexterity * 0.5) + (skill_level * 0.3)
    
    def calculate_base_damage(self, attacker: CharacterStats, weapon_id: str) -> int:
        """基本ダメージ計算"""
        is_unarmed = not weapon_id or weapon_id == "unarmed"
        
        if is_unarmed:
            # 素手戦闘: 3 + (格闘スキル × 0.8) + (力 × 0.7)
            combat_skill = self.get_skill_level(attacker, SkillType.Combat)
            base_unarmed_damage = self.get_weapon_attack_power(weapon_id)  # 3
            
            unarmed_damage = base_unarmed_damage + (combat_skill * 0.8) + (attacker.strength * 0.7)
            return max(1, int(unarmed_damage))
        else:
            # 武器戦闘
            weapon_skill = self.get_weapon_skill_type(weapon_id)
            skill_level = self.get_skill_level(attacker, weapon_skill)
            weapon_attack_power = self.get_weapon_attack_power(weapon_id)
            
            # 武器ダメージ = 武器攻撃力 × (1 + (スキルレベル ÷ 20))
            weapon_damage = weapon_attack_power * (1.0 + (skill_level / 20.0))
            
            # 能力補正
            if self.is_ranged_weapon(weapon_id):
                ability_modifier = attacker.dexterity * 0.5
            else:
                ability_modifier = attacker.strength * 0.5
            
            return max(1, int(weapon_damage + ability_modifier))
    
    def calculate_defense_value(self, defender: CharacterStats) -> int:
        """防御値計算（防具防御力追加）"""
        return max(0, int(defender.armor_defense + (defender.toughness * 0.3)))
    
    def calculate_final_damage(self, base_damage: int, defense_value: int, parried: bool, critical: bool) -> int:
        """最終ダメージ計算"""
        final_damage = float(base_damage)
        
        # クリティカルヒット処理
        if critical:
            final_damage *= 2.0
        
        # 受け流し処理（80%カット）
        if parried:
            final_damage *= 0.2
        
        # 防御計算: 最終ダメージ = 基本ダメージ × (100 ÷ (100 + 防御値))
        final_damage = final_damage * (100.0 / (100.0 + defense_value))
        
        return max(1, int(final_damage))
    
    def perform_combat_calculation(self, attacker: CharacterStats, defender: CharacterStats, weapon_id: str) -> CombatResult:
        """戦闘計算の実行"""
        result = CombatResult()
        
        # 各種確率計算
        hit_chance = self.calculate_hit_chance(attacker, weapon_id)
        dodge_chance = self.calculate_dodge_chance(defender)
        parry_chance = self.calculate_parry_chance(defender)
        critical_chance = self.calculate_critical_chance(attacker, weapon_id)
        
        # 判定順序：回避 → 命中 → 受け流し → クリティカル
        
        # 1. 回避判定
        if random.random() * 100.0 < dodge_chance:
            result.dodged = True
            result.hit = False
            result.final_damage = 0
            return result
        
        # 2. 命中判定
        if random.random() * 100.0 >= hit_chance:
            result.hit = False
            result.final_damage = 0
            return result
        
        result.hit = True
        
        # 3. 受け流し判定
        if random.random() * 100.0 < parry_chance:
            result.parried = True
        
        # 4. クリティカル判定
        if random.random() * 100.0 < critical_chance:
            result.critical = True
        
        # 5. ダメージ計算
        result.base_damage = self.calculate_base_damage(attacker, weapon_id)
        defense_value = self.calculate_defense_value(defender)
        result.final_damage = self.calculate_final_damage(result.base_damage, defense_value, result.parried, result.critical)
        
        return result
    
    def simulate_single_combat(self, player: CharacterStats, enemy: CharacterStats, max_rounds: int = 1000) -> Dict:
        """単一戦闘をシミュレート"""
        # HP設定
        player_hp = player.calculate_health()
        enemy_hp = enemy.calculate_health()
        
        rounds = 0
        total_player_damage = 0
        total_enemy_damage = 0
        
        while player_hp > 0 and enemy_hp > 0 and rounds < max_rounds:
            rounds += 1
            
            # プレイヤーの攻撃
            if player_hp > 0:
                result = self.perform_combat_calculation(player, enemy, player.weapon_id)
                if result.final_damage > 0:
                    enemy_hp = max(0, enemy_hp - result.final_damage)
                    total_player_damage += result.final_damage
            
            # 敵の攻撃
            if enemy_hp > 0:
                result = self.perform_combat_calculation(enemy, player, enemy.weapon_id)
                if result.final_damage > 0:
                    player_hp = max(0, player_hp - result.final_damage)
                    total_enemy_damage += result.final_damage
        
        winner = "player" if player_hp > 0 else "enemy"
        if player_hp <= 0 and enemy_hp <= 0:
            winner = "draw"
        
        return {
            "winner": winner,
            "rounds": rounds,
            "player_damage_dealt": total_player_damage,
            "enemy_damage_dealt": total_enemy_damage,
        }
    
    def simulate_multiple_combats(self, player: CharacterStats, enemy: CharacterStats, iterations: int = 100) -> Dict:
        """複数戦闘の統計を取る"""
        results = []
        
        for _ in range(iterations):
            result = self.simulate_single_combat(player, enemy)
            results.append(result)
        
        # 統計計算
        player_wins = sum(1 for r in results if r["winner"] == "player")
        
        return {
            "player_win_rate": player_wins / iterations * 100,
            "avg_player_damage": sum(r["player_damage_dealt"] for r in results) / len(results),
            "avg_enemy_damage": sum(r["enemy_damage_dealt"] for r in results) / len(results),
        }


def generate_balanced_player(club_type: ClubType = ClubType.Baseball, weapon_id: str = "", armor_defense: float = 0.0) -> CharacterStats:
    """バランス調整版プレイヤー生成"""
    
    # 基本能力値 (1-12)
    base_strength = random.uniform(1, 12)
    base_toughness = random.uniform(1, 12)
    base_intelligence = random.uniform(1, 12)
    base_dexterity = random.uniform(1, 12)
    base_agility = random.uniform(1, 12)
    base_willpower = random.uniform(1, 12)
    
    # 部活ボーナス（37.5%）
    if club_type == ClubType.Baseball:
        strength_bonus = random.uniform(5.625 * 0.5, 5.625)
        toughness_bonus = random.uniform(5.625 * 0.5, 5.625)
        intelligence_bonus = random.uniform(1.875 * 0.5, 1.875)
        dexterity_bonus = random.uniform(5.625 * 0.5, 5.625)
        agility_bonus = random.uniform(5.625 * 0.5, 5.625)
        willpower_bonus = 0
        combat_skill_bonus = random.uniform(1.875 * 0.5, 1.875)
    elif club_type == ClubType.Kendo:
        strength_bonus = random.uniform(3.75 * 0.5, 3.75)
        toughness_bonus = random.uniform(3.75 * 0.5, 3.75)
        intelligence_bonus = 0
        dexterity_bonus = random.uniform(3.75 * 0.5, 3.75)
        agility_bonus = random.uniform(5.625 * 0.5, 5.625)
        willpower_bonus = random.uniform(7.5 * 0.5, 7.5)
        combat_skill_bonus = 0
    else:
        strength_bonus = toughness_bonus = intelligence_bonus = 0
        dexterity_bonus = agility_bonus = willpower_bonus = 0
        combat_skill_bonus = 0
    
    # 最終能力値
    final_strength = min(100, base_strength + strength_bonus)
    final_toughness = min(100, base_toughness + toughness_bonus)
    final_intelligence = min(100, base_intelligence + intelligence_bonus)
    final_dexterity = min(100, base_dexterity + dexterity_bonus)
    final_agility = min(100, base_agility + agility_bonus)
    final_willpower = min(100, base_willpower + willpower_bonus)
    
    # スキル値 (1-20 + ボーナス)
    base_combat_skill = random.uniform(1, 20)
    final_combat_skill = min(100, base_combat_skill + combat_skill_bonus)
    
    # 武器に応じたスキル追加
    skills = {SkillType.Combat: final_combat_skill, SkillType.Evasion: random.uniform(1, 20)}
    
    if weapon_id in ["short_sword", "iron_sword", "steel_sword", "magic_sword", "short_bow", "long_bow"]:
        skills[SkillType.OneHandedWeapons] = random.uniform(1, 20)
    elif weapon_id in ["great_sword", "war_hammer"]:
        skills[SkillType.TwoHandedWeapons] = random.uniform(1, 20)
    
    return CharacterStats(
        name="プレイヤー",
        strength=final_strength,
        toughness=final_toughness,
        intelligence=final_intelligence,
        dexterity=final_dexterity,
        agility=final_agility,
        willpower=final_willpower,
        skills=skills,
        weapon_id=weapon_id,
        armor_defense=armor_defense
    )


def create_enemy(power_level: int, weapon_id: str = "", armor_defense: float = 0.0) -> CharacterStats:
    """敵作成（力レベル別）"""
    
    enemy_configs = {
        7: {
            "name": "弱い敵（ネズミ）",
            "base_stats": (3, 2, 1, 4, 5, 3),  # 力、頑丈、知能、器用、敏捷、意志
            "skills": {SkillType.Combat: 3, SkillType.Evasion: 3}
        },
        15: {
            "name": "ゴブリン", 
            "base_stats": (15, 16, 12, 16, 17, 14),
            "skills": {SkillType.OneHandedWeapons: 10, SkillType.Evasion: 8}
        },
        20: {
            "name": "強いゴブリン",
            "base_stats": (20, 22, 15, 20, 22, 18),
            "skills": {SkillType.OneHandedWeapons: 15, SkillType.Evasion: 12}
        },
        30: {
            "name": "オーク",
            "base_stats": (30, 35, 10, 15, 18, 25),
            "skills": {SkillType.OneHandedWeapons: 20, SkillType.Evasion: 8}
        },
        40: {
            "name": "オーガ",
            "base_stats": (40, 45, 8, 12, 15, 30),
            "skills": {SkillType.TwoHandedWeapons: 25, SkillType.Evasion: 5}
        }
    }
    
    config = enemy_configs[power_level]
    stats = config["base_stats"]
    
    return CharacterStats(
        name=config["name"],
        strength=stats[0], toughness=stats[1], intelligence=stats[2],
        dexterity=stats[3], agility=stats[4], willpower=stats[5],
        skills=config["skills"],
        weapon_id=weapon_id,
        armor_defense=armor_defense
    )


def comprehensive_balance_test():
    """包括的バランステスト"""
    print("=== 包括的バランステスト ===")
    print("プレイヤー: 調整済み能力値(1-12 + 部活ボーナス37.5%)")
    print("敵レベル: 7(ネズミ), 15(ゴブリン), 20(強ゴブリン), 30(オーク), 40(オーガ)")
    print("武器: 素手, ショートソード, 鋼の剣, 魔法の剣, 大剣")
    print()
    
    simulator = CombatSimulator()
    
    # テスト用武器リスト
    weapons = [
        ("素手", ""),
        ("ショートソード", "short_sword"),
        ("鋼の剣", "steel_sword"),
        ("魔法の剣", "magic_sword"),
        ("大剣", "great_sword"),
    ]
    
    # 敵レベルリスト
    enemy_levels = [7, 15, 20, 30, 40]
    
    # 結果格納用
    results = {}
    
    for weapon_name, weapon_id in weapons:
        results[weapon_name] = {}
        
        print(f"\n【武器: {weapon_name}】")
        print("-" * 60)
        
        for enemy_level in enemy_levels:
            # プレイヤー生成（武器に応じた部活選択）
            club = ClubType.Kendo if weapon_id in ["short_sword", "steel_sword", "magic_sword", "great_sword"] else ClubType.Baseball
            
            win_rates = []
            for _ in range(5):  # 5回テスト
                player = generate_balanced_player(club, weapon_id)
                enemy = create_enemy(enemy_level, "short_sword" if enemy_level >= 15 else "")
                
                result = simulator.simulate_multiple_combats(player, enemy, 50)
                win_rates.append(result["player_win_rate"])
            
            avg_win_rate = sum(win_rates) / len(win_rates)
            results[weapon_name][enemy_level] = avg_win_rate
            
            # 勝率に応じた評価
            if 45 <= avg_win_rate <= 55:
                status = "✅ 理想的"
            elif 40 <= avg_win_rate <= 60:
                status = "🟡 良好"
            elif avg_win_rate > 70:
                status = "🔴 プレイヤー有利"
            elif avg_win_rate < 30:
                status = "🔵 敵有利"
            else:
                status = "🟠 要調整"
            
            print(f"  vs 敵レベル{enemy_level:2d}: {avg_win_rate:5.1f}% {status}")
    
    # 総合結果表示
    print(f"\n{'='*80}")
    print("総合バランス表 (プレイヤー勝率%)")
    print(f"{'='*80}")
    print(f"{'武器/敵レベル':<12} | {'7':<6} | {'15':<6} | {'20':<6} | {'30':<6} | {'40':<6}")
    print("-" * 60)
    
    for weapon_name in results:
        line = f"{weapon_name:<12} |"
        for enemy_level in enemy_levels:
            win_rate = results[weapon_name][enemy_level]
            line += f" {win_rate:5.1f} |"
        print(line)
    
    # バランス評価
    print(f"\n{'='*80}")
    print("バランス評価:")
    print("✅ 理想的 (45-55%): 互角の戦い")
    print("🟡 良好 (40-60%): バランス取れている") 
    print("🔴 プレイヤー有利 (70%+): 敵を強化する必要")
    print("🔵 敵有利 (30%-): プレイヤーを強化する必要")
    print("🟠 要調整 (その他): バランス調整が必要")


if __name__ == "__main__":
    comprehensive_balance_test()