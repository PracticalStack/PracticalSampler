Engine::getProjectInfo() -> JSON

Thread safety: UNSAFE -- allocates DynamicObject, constructs String properties
Returns JSON object with: Company, CompanyURL, CompanyCopyright, ProjectName,
ProjectVersion, EncryptionKey, HISEBuild, BuildDate, LicensedEmail.
LicensedEmail only populated when USE_BACKEND or USE_COPY_PROTECTION is enabled.
Pair with:
  getName/getVersion -- shortcut to individual properties
Source:
  ScriptingApi.cpp  Engine::getProjectInfo()
    -> [backend] GET_HISE_SETTING()
