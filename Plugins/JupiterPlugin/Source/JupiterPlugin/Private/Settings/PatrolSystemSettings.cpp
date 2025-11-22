#include "Settings/PatrolSystemSettings.h"

#define LOCTEXT_NAMESPACE "PatrolSystemSettings"

UPatrolSystemSettings::UPatrolSystemSettings()
{
	// Default values are already set in the header
}

FName UPatrolSystemSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FText UPatrolSystemSettings::GetSectionText() const
{
	return LOCTEXT("PatrolSystemSettingsSection", "Patrol System");
}

const UPatrolSystemSettings* UPatrolSystemSettings::Get()
{
	return GetDefault<UPatrolSystemSettings>();
}

#undef LOCTEXT_NAMESPACE
