#pragma once
#include <cereal/archives/json.hpp>

#define GEE_SERIALIZATION_INST_SAVE(Type) template void Type::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const
#define GEE_SERIALIZATION_INST_LOAD(Type) template void Type::Load<cereal::JSONInputArchive>(cereal::JSONInputArchive&)
#define GEE_SERIALIZATION_INST_LOAD_AND_CONSTRUCT(Type) template void Type::load_and_construct<cereal::JSONInputArchive>(cereal::JSONInputArchive&, cereal::construct<Type>&)

#define GEE_SERIALIZATION_INST_DEFAULT(Type) GEE_SERIALIZATION_INST_SAVE(Type); GEE_SERIALIZATION_INST_LOAD(Type);

