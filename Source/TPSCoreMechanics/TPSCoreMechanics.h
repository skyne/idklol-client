// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// custom log category
TPSCOREMECHANICS_API DECLARE_LOG_CATEGORY_EXTERN(Idklol, Log, All);

// custom log macro
#define LOG(x, ...) UE_LOG(Idklol, Log, TEXT(x), ##__VA_ARGS__)
#define LOG_DEBUG(x, ...) UE_LOG(Idklol, Verbose, TEXT(x), ##__VA_ARGS__)
#define LOG_WARNING(x, ...) UE_LOG(Idklol, Warning, TEXT(x), ##__VA_ARGS__)
#define LOG_ERROR(x, ...) UE_LOG(Idklol, Error, TEXT(x), ##__VA_ARGS__)