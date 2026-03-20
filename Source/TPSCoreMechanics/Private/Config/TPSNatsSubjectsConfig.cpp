#include "Config/TPSNatsSubjectsConfig.h"

namespace
{
	FString ResolveTemplateSubject(const FString& SubjectTemplate, const FString& InstanceId)
	{
		if (SubjectTemplate.Contains(TEXT("%s")))
		{
			return SubjectTemplate.Replace(TEXT("%s"), *InstanceId);
		}

		return SubjectTemplate;
	}
}

const UTPSNatsSubjectsConfig& UTPSNatsSubjectsConfig::Get()
{
	return *GetDefault<UTPSNatsSubjectsConfig>();
}

FString UTPSNatsSubjectsConfig::MakeServerMapSubject(const FString& InstanceId) const
{
	return ResolveTemplateSubject(ServerMapSubjectTemplate, InstanceId);
}

FString UTPSNatsSubjectsConfig::MakeServerStatusSubject(const FString& InstanceId) const
{
	return ResolveTemplateSubject(ServerStatusSubjectTemplate, InstanceId);
}
