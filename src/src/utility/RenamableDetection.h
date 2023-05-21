#pragma once
#include <type_traits>
#include <utility/Utility.h>

namespace GEE
{
    // Based on jrok's answer https://stackoverflow.com/questions/87372/check-if-a-class-has-a-member-function-of-a-given-signature.

    // Primary template with a static assertion
    // for a meaningful error message
    // if it ever gets instantiated.
    // We could leave it undefined if we didn't care.



#define GENERATE_METHOD_CHECKER(MethodName) \
\
    template<typename, typename T> \
    struct has_##MethodName { \
        static_assert( \
            std::integral_constant<T, false>::value,\
            "Second template parameter needs to be of function type."); \
    }; \
\
    \
    template<typename C, typename Ret, typename... Args>\
        struct has_##MethodName <C, Ret(Args...)> {\
    private:\
        template<typename T>                                                                         \
        static constexpr auto check(T*)                                                              \
            -> typename                                                                              \
            std::is_same<                                                                            \
        decltype(std::declval<T>().##MethodName(std::declval<Args>()...)),                            \
            Ret    \
            >::type;  \
    \
        template<typename>                                                                           \
        static constexpr std::false_type check(...);                                                 \
        \
        typedef decltype(check<C>(0)) type;                                                          \
        \
        public:                                                                                          \
        static constexpr bool value = type::value;                                                   \
    }; 

        GENERATE_METHOD_CHECKER(SetName);
        GENERATE_METHOD_CHECKER(GetName);

        template <typename T> bool is_renamable() {
            return true;
            return has_GetName<T, String() const>::value && has_SetName<T, void(const String&)>::value;
        }
}