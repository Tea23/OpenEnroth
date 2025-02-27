#pragma once

#include <climits>
#include <string>

#include "Library/Serialization/Serialization.h"

#include "ConfigFwd.h"
#include "AbstractConfigValue.h"

template <class T>
class ConfigValue : public AbstractConfigValue {
 public:
    using validator_type = T (*)(T);

    ConfigValue(const ConfigValue &other) = delete; // non-copyable
    ConfigValue(ConfigValue&& other) = delete; // non-movable

    ConfigValue(ConfigSection *section, const std::string &name, T defaultValue, validator_type validator, const std::string &description) :
        AbstractConfigValue(section, name, description), defaultValue_(defaultValue), value_(defaultValue), validator_(validator) {}

    ConfigValue(ConfigSection *section, const std::string &name, T defaultValue, const std::string &description) :
        ConfigValue(section, name, defaultValue, nullptr, description) {}

    const T &Default() const {
        return defaultValue_;
    }

    const T &Get() const {
        return value_;
    }

    T Set(const T &val) {
        if (validator_)
            value_ = validator_(val);
        else
            value_ = val;

        return value_;
    }

    virtual std::string GetString() const override {
        return toString(value_);
    }

    virtual std::string DefaultString() const override {
        return toString(defaultValue_);
    }

    virtual void SetString(const std::string &value) override {
        value_ = fromString<T>(value);
    }

    virtual void Reset() override {
        value_ = defaultValue_;
    }

    T Toggle() requires std::is_same_v<T, bool> {
        value_ = !value_;

        return value_;
    }

    T Increment() requires std::is_same_v<T, int> {
        if (validator_)
            value_ = validator_(value_ + 1);
        else
            value_++;

        return value_;
    }

    T Decrement() requires std::is_same_v<T, int> {
        if (validator_)
            value_ = validator_(value_ - 1);
        else
            value_--;

        return value_;
    }

    T CycleIncrement() requires std::is_same_v<T, int> {
        // we rely on validator with std::clamp-like behaviour.
        assert(validator_);

        int old = value_;
        value_ = validator_(value_ + 1);
        if (value_ == old)
            value_ = validator_(INT_MIN);

        return value_;
    }

    T CycleDecrement() requires std::is_same_v<T, int> {
        // we rely on validator with std::clamp-like behaviour.
        assert(validator_);

        int old = value_;
        value_ = validator_(value_ - 1);
        if (value_ == old)
            value_ = validator_(INT_MAX);

        return value_;
    }

 private:
    T value_;
    T defaultValue_;
    validator_type validator_;
};
