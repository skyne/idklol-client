#include "Helpers/JsonObjectUtils.h"

#include "Serialization/JsonSerializer.h"

bool TPSCoreJson::DeserializeObject(const FString& Json, TSharedPtr<FJsonObject>& OutRoot)
{
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	return FJsonSerializer::Deserialize(Reader, OutRoot) && OutRoot.IsValid();
}

FString TPSCoreJson::SerializeObject(const TSharedRef<FJsonObject>& JsonObject)
{
	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(JsonObject, Writer);
	return Output;
}
