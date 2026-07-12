Expansion::getProperties() -> JSON

Thread safety: UNSAFE -- constructs DynamicObject from internal ValueTree (heap allocation, string construction)
Returns a JSON object with expansion metadata. Standard properties: Name (defaults to
folder name), Version ("1.0.0"), Tags (""), ProjectName, ProjectVersion. Additional
properties (Description, Company, URL, UUID, etc.) may be present from expansion_info.xml.
Returns undefined if the expansion reference has been invalidated.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Dispatch/mechanics:
  Expansion::getPropertyObject() -> Data::toPropertyObject()
    -> ValueTreeConverters::convertValueTreeToDynamicObject(v)

Source:
  ExpansionHandler.cpp:867  Expansion::getPropertyObject()
    -> Data::toPropertyObject() at line 1336
    -> converts CachedValue-backed ValueTree to DynamicObject
