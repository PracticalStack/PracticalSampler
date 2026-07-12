FixObjectFactory::getTypeHash() -> Integer

Thread safety: SAFE
Returns an integer hash computed from member names and data types during
construction. Two factories with identical layout descriptions (same property
names, types, and order) produce the same hash. Useful for verifying type
compatibility between factories or containers at runtime.

Source:
  FixLayoutObjects.h  Factory::getTypeHash() -> returns typeHash (inline)
  FixLayoutObjects.cpp  Helpers::createHash()
    -> concatenates member id + (uint8)type for each layout item
    -> returns String::hashCode()
