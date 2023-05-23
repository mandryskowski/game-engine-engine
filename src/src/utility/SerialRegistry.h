#pragma once
#include <cereal/types/polymorphic.hpp>

#define GEE_REGISTER_TYPE(Type) CEREAL_REGISTER_TYPE(Type)
#define GEE_REGISTER_POLYMORPHIC_RELATION(Base, Derived) CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, Derived)
#define GEE_REGISTER_POLYMORPHIC_TYPE(Base, Derived) GEE_REGISTER_TYPE(Derived); \
													 CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, Derived)
