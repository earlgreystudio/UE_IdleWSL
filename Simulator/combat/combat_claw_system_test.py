#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
爪牙システムの詳細テスト
動物の爪vs人間の武器のバランスを詳しく検証
"""

import random
import math
from dataclasses import dataclass
from typing import List, Dict, Tuple
from enum import Enum


class SkillType(Enum):
    OneHandedWeapons = "OneHandedWeapons"
    Combat = "Combat"
    Evasion = "Evasion"


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
    
    def calculate_health(self) -> float:
        return (self.toughness * 3.0) + (self.strength * 1.0) + 50.0


@dataclass
class WeaponData:
    name: str
    attack_power: int
    weight: float
    skill_type: SkillType


class CombatSimulator:
    def __init__(self, use_claw_system: bool = False):
        self.use_claw_system = use_claw_system
        self.weapons = self._create_weapon_database()
    
    def _create_weapon_database(self) -> Dict:
        if self.use_claw_system:
            # 爪牙システム使用
            return {
                # 人間用
                "": WeaponData("人間の素手", 2, 0.5, SkillType.Combat),
                "unarmed": WeaponData("人間の素手", 2, 0.5, SkillType.Combat),
                "short_sword": WeaponData("ショートソード", 8, 1.2, SkillType.OneHandedWeapons),
                "steel_sword": WeaponData("鋼の剣", 15, 1.8, SkillType.OneHandedWeapons),
                "magic_sword": WeaponData("魔法の剣", 20, 1.0, SkillType.OneHandedWeapons),
                
                # 動物用（爪牙）
                "rat_bite": WeaponData("ネズミの歯", 4, 0.3, SkillType.Combat),
                "goblin_claw": WeaponData("ゴブリンの爪", 6, 0.5, SkillType.Combat),
                "wolf_fang": WeaponData("狼の牙", 10, 0.5, SkillType.Combat),
                "bear_claw": WeaponData("クマの爪", 15, 0.5, SkillType.Combat),
                "dragon_breath": WeaponData("ドラゴンブレス", 25, 0.5, SkillType.Combat),
                "orc_fist": WeaponData("オークの拳", 12, 0.8, SkillType.Combat),
            }
        else:
            # 従来システム
            return {
                "": WeaponData("素手", 3, 0.5, SkillType.Combat),
                "unarmed": WeaponData("素手", 3, 0.5, SkillType.Combat),
                "short_sword": WeaponData("ショートソード", 8, 1.2, SkillType.OneHandedWeapons),
                "steel_sword": WeaponData("鋼の剣", 15, 1.8, SkillType.OneHandedWeapons),
                "magic_sword": WeaponData("魔法の剣", 20, 1.0, SkillType.OneHandedWeapons),
            }
    
    def get_skill_level(self, character: CharacterStats, skill_type: SkillType) -> float:
        return character.skills.get(skill_type, 1.0)
    
    def get_weapon_skill_type(self, weapon_id: str) -> SkillType:
        if not weapon_id or weapon_id == "unarmed":
            return SkillType.Combat
        return self.weapons.get(weapon_id, self.weapons[""]).skill_type
    
    def get_weapon_attack_power(self, weapon_id: str) -> int:
        if not weapon_id or weapon_id == "unarmed":
            return self.weapons[""].attack_power
        return self.weapons.get(weapon_id, self.weapons[""]).attack_power
    
    def calculate_base_damage(self, attacker: CharacterStats, weapon_id: str) -> int:
        """基本ダメージ計算"""
        is_unarmed_or_natural = (not weapon_id or weapon_id == "unarmed" or 
                                weapon_id in ["rat_bite", "goblin_claw", "wolf_fang", "bear_claw", 
                                            "dragon_breath", "orc_fist"])
        
        if is_unarmed_or_natural:
            # 素手戦闘・自然武器
            combat_skill = self.get_skill_level(attacker, SkillType.Combat)
            base_damage = self.get_weapon_attack_power(weapon_id)
            
            # 従来の計算式
            total_damage = base_damage + (combat_skill * 0.8) + (attacker.strength * 0.7)
            return max(1, int(total_damage))
        else:
            # 人工武器
            weapon_skill = self.get_weapon_skill_type(weapon_id)
            skill_level = self.get_skill_level(attacker, weapon_skill)
            weapon_attack_power = self.get_weapon_attack_power(weapon_id)
            
            weapon_damage = weapon_attack_power * (1.0 + (skill_level / 20.0))
            ability_modifier = attacker.strength * 0.5
            
            return max(1, int(weapon_damage + ability_modifier))
    
    def calculate_hit_chance(self, attacker: CharacterStats, weapon_id: str) -> float:
        weapon_skill = self.get_weapon_skill_type(weapon_id)
        skill_level = self.get_skill_level(attacker, weapon_skill)
        return 70.0 + (skill_level * 1.0)  # 簡略化
    
    def calculate_dodge_chance(self, defender: CharacterStats) -> float:
        evasion_skill = self.get_skill_level(defender, SkillType.Evasion)
        return 10.0 + (defender.agility * 1.0) + (evasion_skill * 2.0)
    
    def simulate_combat(self, player: CharacterStats, enemy: CharacterStats, iterations: int = 100) -> Dict:
        player_wins = 0
        total_player_damage = 0
        total_enemy_damage = 0
        
        for _ in range(iterations):
            player_hp = player.calculate_health()
            enemy_hp = enemy.calculate_health()
            
            round_player_damage = 0
            round_enemy_damage = 0
            
            rounds = 0
            while player_hp > 0 and enemy_hp > 0 and rounds < 1000:
                rounds += 1
                
                # プレイヤー攻撃
                if player_hp > 0:
                    if random.random() * 100.0 >= self.calculate_dodge_chance(enemy):
                        if random.random() * 100.0 < self.calculate_hit_chance(player, player.weapon_id):
                            damage = self.calculate_base_damage(player, player.weapon_id)
                            enemy_hp = max(0, enemy_hp - damage)
                            round_player_damage += damage
                
                # 敵攻撃
                if enemy_hp > 0:
                    if random.random() * 100.0 >= self.calculate_dodge_chance(player):
                        if random.random() * 100.0 < self.calculate_hit_chance(enemy, enemy.weapon_id):
                            damage = self.calculate_base_damage(enemy, enemy.weapon_id)
                            player_hp = max(0, player_hp - damage)
                            round_enemy_damage += damage
            
            if player_hp > 0:
                player_wins += 1
            
            total_player_damage += round_player_damage
            total_enemy_damage += round_enemy_damage
        
        return {
            "win_rate": player_wins / iterations * 100,
            "avg_player_damage": total_player_damage / iterations,
            "avg_enemy_damage": total_enemy_damage / iterations
        }


def generate_player(weapon_id: str = "") -> CharacterStats:
    """プレイヤー生成"""
    # バランス調整済み値
    strength = random.uniform(8, 15)
    toughness = random.uniform(8, 15)
    dexterity = random.uniform(8, 15)
    agility = random.uniform(8, 15)
    
    skills = {SkillType.Combat: random.uniform(8, 18), SkillType.Evasion: random.uniform(5, 15)}
    
    if weapon_id in ["short_sword", "steel_sword", "magic_sword"]:
        skills[SkillType.OneHandedWeapons] = random.uniform(8, 18)
    
    return CharacterStats(
        name="プレイヤー",
        strength=strength, toughness=toughness, intelligence=10,
        dexterity=dexterity, agility=agility, willpower=10,
        skills=skills, weapon_id=weapon_id
    )


def create_enemy(enemy_type: str, weapon_id: str) -> CharacterStats:
    """敵作成"""
    enemy_configs = {
        "rat": {
            "name": "ネズミ",
            "stats": (3, 2, 1, 4, 5, 3),
            "skills": {SkillType.Combat: 3, SkillType.Evasion: 8}
        },
        "goblin": {
            "name": "ゴブリン", 
            "stats": (15, 16, 12, 16, 17, 14),
            "skills": {SkillType.Combat: 8, SkillType.OneHandedWeapons: 10, SkillType.Evasion: 8}
        },
        "wolf": {
            "name": "狼",
            "stats": (18, 12, 8, 15, 22, 12),
            "skills": {SkillType.Combat: 12, SkillType.Evasion: 15}
        },
        "bear": {
            "name": "クマ",
            "stats": (25, 20, 6, 10, 12, 15),
            "skills": {SkillType.Combat: 15, SkillType.Evasion: 5}
        },
        "orc": {
            "name": "オーク",
            "stats": (22, 18, 8, 12, 15, 14),
            "skills": {SkillType.Combat: 12, SkillType.OneHandedWeapons: 15, SkillType.Evasion: 6}
        }
    }
    
    config = enemy_configs[enemy_type]
    stats = config["stats"]
    
    return CharacterStats(
        name=config["name"],
        strength=stats[0], toughness=stats[1], intelligence=stats[2],
        dexterity=stats[3], agility=stats[4], willpower=stats[5],
        skills=config["skills"], weapon_id=weapon_id
    )


def test_claw_system_detailed():
    """爪牙システムの詳細テスト"""
    print("=== 爪牙システム詳細テスト ===")
    print("従来システム vs 爪牙システムの比較")
    print("各敵が適切な自然武器を使用")
    print()
    
    # テスト設定
    enemy_setups = [
        ("rat", "rat_bite", "ネズミ（歯で噛む）"),
        ("goblin", "goblin_claw", "ゴブリン（爪で引っかく）"),
        ("goblin", "short_sword", "ゴブリン（剣を装備）"),
        ("wolf", "wolf_fang", "狼（牙で噛む）"),
        ("bear", "bear_claw", "クマ（爪で引っかく）"),
        ("orc", "orc_fist", "オーク（拳で殴る）"),
    ]
    
    player_weapons = [
        ("素手", ""),
        ("ショートソード", "short_sword"),
        ("鋼の剣", "steel_sword"),
        ("魔法の剣", "magic_sword"),
    ]
    
    print("=== 従来システム ===")
    simulator_old = CombatSimulator(use_claw_system=False)
    
    for enemy_type, enemy_weapon, enemy_desc in enemy_setups:
        if enemy_weapon in ["rat_bite", "goblin_claw", "wolf_fang", "bear_claw", "orc_fist"]:
            continue  # 従来システムでは自然武器は存在しない
        
        print(f"\n【{enemy_desc}】")
        for player_weapon_name, player_weapon_id in player_weapons[:2]:  # 素手とショートソードのみ
            player = generate_player(player_weapon_id)
            enemy = create_enemy(enemy_type, enemy_weapon)
            
            # 基本ダメージ計算を表示
            player_damage = simulator_old.calculate_base_damage(player, player.weapon_id)
            enemy_damage = simulator_old.calculate_base_damage(enemy, enemy.weapon_id)
            
            result = simulator_old.simulate_combat(player, enemy, 50)
            
            print(f"  {player_weapon_name:<12}: 勝率{result['win_rate']:5.1f}% | プレイヤーダメージ{player_damage:2d} vs 敵ダメージ{enemy_damage:2d}")
    
    print(f"\n{'='*80}")
    print("=== 爪牙システム ===")
    simulator_new = CombatSimulator(use_claw_system=True)
    
    for enemy_type, enemy_weapon, enemy_desc in enemy_setups:
        print(f"\n【{enemy_desc}】")
        for player_weapon_name, player_weapon_id in player_weapons:
            player = generate_player(player_weapon_id)
            enemy = create_enemy(enemy_type, enemy_weapon)
            
            # 基本ダメージ計算を表示
            player_damage = simulator_new.calculate_base_damage(player, player.weapon_id)
            enemy_damage = simulator_new.calculate_base_damage(enemy, enemy.weapon_id)
            
            result = simulator_new.simulate_combat(player, enemy, 50)
            
            # 評価
            if 45 <= result['win_rate'] <= 55:
                status = "✅"
            elif 40 <= result['win_rate'] <= 60:
                status = "🟡"
            elif result['win_rate'] > 70:
                status = "🔴"
            elif result['win_rate'] < 30:
                status = "🔵"
            else:
                status = "🟠"
            
            print(f"  {player_weapon_name:<12}: 勝率{result['win_rate']:5.1f}% {status} | プレイヤーダメージ{player_damage:2d} vs 敵ダメージ{enemy_damage:2d}")
    
    print(f"\n{'='*80}")
    print("評価基準:")
    print("✅ 理想的 (45-55%) | 🟡 良好 (40-60%) | 🔴 プレイヤー有利 (70%+) | 🔵 敵有利 (30%-) | 🟠 要調整")
    
    print(f"\n爪牙システムの利点:")
    print("1. 動物が適切な強さの自然武器を持つ")
    print("2. 人間の素手は弱く、武器の価値が明確") 
    print("3. 各動物の特性に合った攻撃力設定")
    print("4. ゴブリンは武器装備 vs 素手の選択肢あり")


if __name__ == "__main__":
    test_claw_system_detailed()