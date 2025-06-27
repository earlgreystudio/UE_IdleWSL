#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/TaskTypes.h"
#include "CraftingComponent.generated.h"

// レシピ情報構造体
USTRUCT(BlueprintType)
struct UE_IDLE_API FCraftableRecipe
{
    GENERATED_BODY()

    // レシピID（内部識別用）
    UPROPERTY(BlueprintReadWrite, Category = "Recipe")
    FString RecipeId;

    // 表示名（日本名）
    UPROPERTY(BlueprintReadWrite, Category = "Recipe")
    FString DisplayName;

    FCraftableRecipe()
    {
        RecipeId = TEXT("");
        DisplayName = TEXT("");
    }

    FCraftableRecipe(const FString& InRecipeId, const FString& InDisplayName)
        : RecipeId(InRecipeId), DisplayName(InDisplayName)
    {
    }
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UCraftingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCraftingComponent();

protected:
    virtual void BeginPlay() override;

    // カテゴリ別の生産可能物リスト
    TMap<ETaskType, TArray<FCraftableRecipe>> CraftableRecipes;

public:
    // 指定カテゴリの生産可能レシピを取得
    UFUNCTION(BlueprintPure, Category = "Crafting")
    TArray<FCraftableRecipe> GetCraftableRecipesByCategory(ETaskType TaskType) const;

    // レシピIDから表示名を取得
    UFUNCTION(BlueprintPure, Category = "Crafting")
    FString GetRecipeDisplayName(const FString& RecipeId, ETaskType TaskType) const;

    // 表示名からレシピIDを取得
    UFUNCTION(BlueprintPure, Category = "Crafting")
    FString GetRecipeIdFromDisplayName(const FString& DisplayName, ETaskType TaskType) const;

    // 指定カテゴリのレシピが存在するかチェック
    UFUNCTION(BlueprintPure, Category = "Crafting")
    bool HasRecipesForCategory(ETaskType TaskType) const;

    // 全カテゴリの表示名リストを取得（UI用）
    UFUNCTION(BlueprintPure, Category = "Crafting")
    TArray<FString> GetDisplayNamesForCategory(ETaskType TaskType) const;

private:
    // 初期レシピデータの設定
    void InitializeDefaultRecipes();

    // カテゴリ別にレシピを追加
    void AddRecipeToCategory(ETaskType TaskType, const FString& RecipeId, const FString& DisplayName);
};