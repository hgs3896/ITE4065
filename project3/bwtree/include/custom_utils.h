#pragma once

#include <string>

namespace util{
    namespace __detail
    {
        template<unsigned... digits>
        struct to_chars { static const char value[]; };

        template<unsigned... digits>
        constexpr char to_chars<digits...>::value[] = {('0' + digits)..., 0};

        template<unsigned rem, unsigned... digits>
        struct explode : explode<rem / 10, rem % 10, digits...> {};

        template<unsigned... digits>
        struct explode<0, digits...> : to_chars<digits...> {};
    }

    template<unsigned num>
    struct num_to_string : __detail::explode<num> {};

    template<size_t value>
    struct Unit{
        static constexpr std::string_view getUnitString() {
          using namespace std::literals::string_literals;
          if constexpr (value < 1024) {
            return num_to_string<value>::to_chars::value;
          } else if (value < 1024 * 1024) {
            return num_to_string<value / 1024>::to_chars::value + "K"s;
          } else if (value < 1024 * 1024 * 1024) {
            return num_to_string<value / (1024 * 1024)>::to_chars::value + "M"s;
          } else {
            return num_to_string<value / (1024 * 1024 * 1024)>::to_chars::value + "G"s;
          }
        }
    };
}