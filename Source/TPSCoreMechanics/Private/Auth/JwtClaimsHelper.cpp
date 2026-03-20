#include "Auth/JwtClaimsHelper.h"

#include "Helpers/JsonObjectUtils.h"
#include "Misc/Base64.h"
#include "Dom/JsonObject.h"

namespace
{
	bool DecodeJwtPayloadToJson(const FString& JwtToken, TSharedPtr<FJsonObject>& OutClaims)
	{
		TArray<FString> Segments;
		JwtToken.ParseIntoArray(Segments, TEXT("."), false);
		if (Segments.Num() < 2)
		{
			return false;
		}

		FString PayloadB64 = Segments[1];
		PayloadB64.ReplaceInline(TEXT("-"), TEXT("+"), ESearchCase::CaseSensitive);
		PayloadB64.ReplaceInline(TEXT("_"), TEXT("/"), ESearchCase::CaseSensitive);

		const int32 Pad = (4 - PayloadB64.Len() % 4) % 4;
		for (int32 I = 0; I < Pad; ++I)
		{
			PayloadB64.AppendChar(TEXT('='));
		}

		TArray<uint8> DecodedBytes;
		if (!FBase64::Decode(PayloadB64, DecodedBytes))
		{
			return false;
		}

		DecodedBytes.Add(0);
		const FString PayloadJson = UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(DecodedBytes.GetData()));

		return TPSCoreJson::DeserializeObject(PayloadJson, OutClaims);
	}
}

bool TPSCoreAuth::ExtractJwtEmailClaim(const FString& JwtToken, FString& OutEmail)
{
	TSharedPtr<FJsonObject> Claims;
	if (!DecodeJwtPayloadToJson(JwtToken, Claims))
	{
		return false;
	}

	return Claims->TryGetStringField(TEXT("email"), OutEmail);
}

bool TPSCoreAuth::ExtractJwtExpirationClaim(const FString& JwtToken, int64& OutExpirationTimestamp)
{
	TSharedPtr<FJsonObject> Claims;
	if (!DecodeJwtPayloadToJson(JwtToken, Claims))
	{
		return false;
	}

	if (!Claims->HasField(TEXT("exp")))
	{
		return false;
	}

	OutExpirationTimestamp = static_cast<int64>(Claims->GetNumberField(TEXT("exp")));
	return true;
}
