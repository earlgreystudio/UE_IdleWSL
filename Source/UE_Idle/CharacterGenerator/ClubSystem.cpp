#include "ClubSystem.h"
#include "Engine/Engine.h"

EClubType UClubSystem::GetRandomClub()
{
    int32 RandomIndex = FMath::RandRange(0, static_cast<int32>(EClubType::Count) - 1);
    return static_cast<EClubType>(RandomIndex);
}