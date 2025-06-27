#include "CraftingComponent.h"

UCraftingComponent::UCraftingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCraftingComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // 初期レシピデータを設定
    InitializeDefaultRecipes();
    
    UE_LOG(LogTemp, Log, TEXT("UCraftingComponent: Initialized with %d categories"), CraftableRecipes.Num());
}

TArray<FCraftableRecipe> UCraftingComponent::GetCraftableRecipesByCategory(ETaskType TaskType) const
{
    if (const TArray<FCraftableRecipe>* Recipes = CraftableRecipes.Find(TaskType))
    {
        return *Recipes;
    }
    
    return TArray<FCraftableRecipe>();
}

FString UCraftingComponent::GetRecipeDisplayName(const FString& RecipeId, ETaskType TaskType) const
{
    if (const TArray<FCraftableRecipe>* Recipes = CraftableRecipes.Find(TaskType))
    {
        for (const FCraftableRecipe& Recipe : *Recipes)
        {
            if (Recipe.RecipeId == RecipeId)
            {
                return Recipe.DisplayName;
            }
        }
    }
    
    return RecipeId; // 見つからない場合はIDをそのまま返す
}

FString UCraftingComponent::GetRecipeIdFromDisplayName(const FString& DisplayName, ETaskType TaskType) const
{
    if (const TArray<FCraftableRecipe>* Recipes = CraftableRecipes.Find(TaskType))
    {
        for (const FCraftableRecipe& Recipe : *Recipes)
        {
            if (Recipe.DisplayName == DisplayName)
            {
                return Recipe.RecipeId;
            }
        }
    }
    
    return DisplayName; // 見つからない場合は表示名をそのまま返す
}

bool UCraftingComponent::HasRecipesForCategory(ETaskType TaskType) const
{
    if (const TArray<FCraftableRecipe>* Recipes = CraftableRecipes.Find(TaskType))
    {
        return Recipes->Num() > 0;
    }
    
    return false;
}

TArray<FString> UCraftingComponent::GetDisplayNamesForCategory(ETaskType TaskType) const
{
    TArray<FString> DisplayNames;
    
    if (const TArray<FCraftableRecipe>* Recipes = CraftableRecipes.Find(TaskType))
    {
        for (const FCraftableRecipe& Recipe : *Recipes)
        {
            DisplayNames.Add(Recipe.DisplayName);
        }
    }
    
    return DisplayNames;
}

void UCraftingComponent::InitializeDefaultRecipes()
{
    // 料理カテゴリ
    AddRecipeToCategory(ETaskType::Cooking, TEXT("cooking_soup"), TEXT("スープ"));
    AddRecipeToCategory(ETaskType::Cooking, TEXT("cooking_bread"), TEXT("パン"));
    AddRecipeToCategory(ETaskType::Cooking, TEXT("cooking_stew"), TEXT("シチュー"));
    AddRecipeToCategory(ETaskType::Cooking, TEXT("cooking_roasted_meat"), TEXT("焼き肉"));
    AddRecipeToCategory(ETaskType::Cooking, TEXT("cooking_fish_dish"), TEXT("魚料理"));

    // 建築カテゴリ
    AddRecipeToCategory(ETaskType::Construction, TEXT("construction_wooden_wall"), TEXT("木の壁"));
    AddRecipeToCategory(ETaskType::Construction, TEXT("construction_stone_foundation"), TEXT("石の基礎"));
    AddRecipeToCategory(ETaskType::Construction, TEXT("construction_wooden_door"), TEXT("木のドア"));
    AddRecipeToCategory(ETaskType::Construction, TEXT("construction_roof_tile"), TEXT("屋根瓦"));
    AddRecipeToCategory(ETaskType::Construction, TEXT("construction_wooden_floor"), TEXT("木の床"));

    // 製作カテゴリ
    AddRecipeToCategory(ETaskType::Crafting, TEXT("crafting_iron_sword"), TEXT("鉄の剣"));
    AddRecipeToCategory(ETaskType::Crafting, TEXT("crafting_leather_armor"), TEXT("革の鎧"));
    AddRecipeToCategory(ETaskType::Crafting, TEXT("crafting_wooden_bow"), TEXT("木の弓"));
    AddRecipeToCategory(ETaskType::Crafting, TEXT("crafting_iron_helmet"), TEXT("鉄の兜"));
    AddRecipeToCategory(ETaskType::Crafting, TEXT("crafting_wooden_shield"), TEXT("木の盾"));

    // 採集カテゴリ（採集対象アイテム）
    AddRecipeToCategory(ETaskType::Gathering, TEXT("wood"), TEXT("木材"));
    AddRecipeToCategory(ETaskType::Gathering, TEXT("stone"), TEXT("石"));
    AddRecipeToCategory(ETaskType::Gathering, TEXT("iron_ore"), TEXT("鉄鉱石"));
    AddRecipeToCategory(ETaskType::Gathering, TEXT("herbs"), TEXT("薬草"));
    AddRecipeToCategory(ETaskType::Gathering, TEXT("berries"), TEXT("ベリー"));

    UE_LOG(LogTemp, Log, TEXT("UCraftingComponent: Initialized default recipes"));
    UE_LOG(LogTemp, Log, TEXT("  - Cooking: %d recipes"), CraftableRecipes[ETaskType::Cooking].Num());
    UE_LOG(LogTemp, Log, TEXT("  - Construction: %d recipes"), CraftableRecipes[ETaskType::Construction].Num());
    UE_LOG(LogTemp, Log, TEXT("  - Crafting: %d recipes"), CraftableRecipes[ETaskType::Crafting].Num());
    UE_LOG(LogTemp, Log, TEXT("  - Gathering: %d items"), CraftableRecipes[ETaskType::Gathering].Num());
}

void UCraftingComponent::AddRecipeToCategory(ETaskType TaskType, const FString& RecipeId, const FString& DisplayName)
{
    if (!CraftableRecipes.Contains(TaskType))
    {
        CraftableRecipes.Add(TaskType, TArray<FCraftableRecipe>());
    }
    
    CraftableRecipes[TaskType].Add(FCraftableRecipe(RecipeId, DisplayName));
}