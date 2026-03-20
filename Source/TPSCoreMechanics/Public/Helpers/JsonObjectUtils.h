#pragma once

#include "CoreMinimal.h"

class FJsonObject;

namespace TPSCoreJson
{
	TPSCOREMECHANICS_API bool DeserializeObject(const FString& Json, TSharedPtr<FJsonObject>& OutRoot);
	TPSCOREMECHANICS_API FString SerializeObject(const TSharedRef<FJsonObject>& JsonObject);
}
