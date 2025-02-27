#pragma once

#include <type_traits>

/**
 * `Flags` is basically an analog of `QFlags` from Qt, a type-safe wrapper
 * for flags defined in enumerations.
 *
 * Example usage:
 *      class MonsterType {
 *      public:
 *          enum class Ability {
 *              FlyingAbility = 0x1,
 *              RunningAbility = 0x2,
 *              ShootingAbility = 0x4
 *          };
 *          using enum Ability;
 *          DECLARE_FLAGS(Abilities, Ability)
 *
 *          // ...
 *      };
 *
 *      DECLARE_OPERATORS_FOR_FLAGS(MonsterType::Abilities)
 */
template <class Enum>
class Flags {
    static_assert(std::is_enum<Enum>::value, "Expecting an enumeration");

    struct Dummy {};
    static void fake_true(Dummy *) {}
    using fake_bool = void (*)(Dummy *);

 public:
    using enumeration_type = Enum;
    using underlying_type = std::underlying_type_t<Enum>;

    constexpr Flags(std::nullptr_t = 0) : // NOLINT: constructor is intentionally implicit
        value_(0)
    {}

    constexpr Flags(enumeration_type value) : // NOLINT: constructor is intentionally implicit
        value_(static_cast<underlying_type>(value))
    {}

    constexpr explicit Flags(underlying_type value) :
        value_(value)
    {}

    constexpr explicit operator underlying_type() const {
        return value_;
    }

    constexpr friend Flags operator|(Flags l, Flags r) {
        return Flags(l.value_ | r.value_);
    }

    constexpr friend Flags operator|(enumeration_type l, Flags r) {
        return Flags(static_cast<underlying_type>(l) | r.value_);
    }

    constexpr friend Flags operator|(Flags l, enumeration_type r) {
        return Flags(l.value_ | static_cast<underlying_type>(r));
    }

    constexpr friend Flags operator&(Flags l, Flags r) {
        return Flags(l.value_ & r.value_);
    }

    constexpr friend Flags operator&(enumeration_type l, Flags r) {
        return Flags(static_cast<underlying_type>(l) & r.value_);
    }

    constexpr friend Flags operator&(Flags l, enumeration_type r) {
        return Flags(l.value_ & static_cast<underlying_type>(r));
    }

    constexpr friend bool operator==(Flags l, Flags r) {
        return l.value_ == r.value_;
    }

    constexpr friend bool operator==(enumeration_type l, Flags r) {
        return static_cast<underlying_type>(l) == r.value_;
    }

    constexpr Flags& operator&=(Flags mask) {
        *this = *this & mask;
        return *this;
    }

    constexpr Flags& operator&=(enumeration_type mask) {
        *this = *this & mask;
        return *this;
    }

    constexpr Flags& operator|=(Flags flags) {
        *this = *this | flags;
        return *this;
    }

    constexpr Flags& operator|=(enumeration_type flags) {
        *this = *this | flags;
        return *this;
    }

    constexpr Flags operator~() const {
        return Flags(~value_);
    }

    constexpr bool operator!() const {
        return !value_;
    }

    /**
     * Implicit operator bool with some dark magic on top.
     *
     * Explicit operator bool doesn't work here because then we can't just write `return flags & SOME_FLAG` in
     * a function returning `bool`.
     *
     * Implicit operator bool allows to do arithmetic on top of the returned result, so something like
     * `return flags == 0x16` actually compiles, albeit in a totally unexpected way (comparing `bool` to
     * `0x16` always returns false).
     *
     * Thus the solution with a function pointer.
     *
     * @returns                         Whether there are no flags set.
     */
    constexpr operator fake_bool() const {
        return value_ ? &fake_true : nullptr;
    }

 private:
    underlying_type value_;
};


#define MM_DECLARE_FLAGS(FLAGS, ENUM)                                           \
    using FLAGS = Flags<ENUM>;


#define MM_DECLARE_OPERATORS_FOR_FLAGS(FLAGS)                                   \
    constexpr inline FLAGS operator|(FLAGS::enumeration_type l, FLAGS::enumeration_type r) { \
        return FLAGS(l) | r;                                                    \
    }                                                                           \
    constexpr inline FLAGS operator~(FLAGS::enumeration_type value) {           \
        return ~FLAGS(value);                                                   \
    }


// TODO(captainurist): technically this is UB
namespace std {
    template<class Enum>
    typename Flags<Enum>::underlying_type to_underlying(Flags<Enum> flags) {
        return static_cast<typename Flags<Enum>::underlying_type>(flags);
    }
}
