#pragma once

#include "CoreMinimal.h"

/**
 * Shared JWT claim helpers used by both client and dedicated-server code.
 *
 * NOTE: These helpers only decode and parse claims from the JWT payload.
 * They do NOT validate cryptographic signatures.
 */
namespace TPSCoreAuth
{
	/** Extract the "email" claim from a JWT payload. */
	TPSCOREMECHANICS_API bool ExtractJwtEmailClaim(const FString& JwtToken, FString& OutEmail);

	/** Extract the "exp" claim (Unix timestamp) from a JWT payload. */
	TPSCOREMECHANICS_API bool ExtractJwtExpirationClaim(const FString& JwtToken, int64& OutExpirationTimestamp);
}
