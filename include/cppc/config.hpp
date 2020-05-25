#ifndef CPP_CONFIG_CONFIG_HPP
#define CPP_CONFIG_CONFIG_HPP

#include <fstream>
#include <array>
#include <unordered_map>
#include <string>
#include <optional>
#include <variant>
#include <algorithm>
#include <type_traits>
#include <cassert>
#include <sstream>
#include <string_view>
#include <functional>

namespace cppc
{
    namespace detail
    {
        template <typename NAME, NAME SIZE = NAME::_SIZE>
        std::unordered_map<std::string, NAME>& getNameToType(){
            static std::unordered_map<std::string, NAME> map;
            return map;
        }

        template <typename NAME, NAME SIZE = NAME::_SIZE>
        std::array<std::string, static_cast<size_t>(SIZE)>& getTypeToName(){
            static std::array<std::string, static_cast<size_t>(SIZE)> array;
            return array;
        }

        inline void trim(std::string &s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
                return !isspace(ch);
            }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
                return !isspace(ch);
            }).base(), s.end());
        }

        inline std::string_view trim(std::string_view s) {
            s.remove_prefix(std::min(s.find_first_not_of(" "), s.size()));
            s.remove_suffix(std::min(s.size() - s.find_last_not_of(" ") - 1, s.size()));
            return s;
        }

        template <typename... T>
        struct AutoConverter : public std::variant<T...> {
            template <typename R>
            operator R&() {
                return std::get<R>(*this);
            }
        };

        template<typename S, std::size_t...Is, typename TUPLE>
        S ToStruct(std::index_sequence<Is...>, TUPLE&& tuple) {
            return S{std::get<Is>(std::forward<TUPLE>(tuple))...};
        }

        template<typename S, typename TUPLE>
        S ToStruct(TUPLE&& tuple) {
            return ToStruct<S>(
                std::make_index_sequence<std::tuple_size<std::remove_reference_t<TUPLE>>{}>{},
                std::forward<TUPLE>(tuple)
            );
        }

/*
        template<size_t N> struct ToTupleImpl;
        template<> struct ToTupleImpl<2> {
            template<typename S> auto operator()(S&& s) const {
                auto[e0, e1] = std::forward<S>(s);
                return std::make_tuple(e0, e1);
            }
        };
        template<> struct ToTupleImpl<3> {
            template<typename S> auto operator()(S&& s) const {
                auto[e0, e1, e2] = std::forward<S>(s);
                return std::make_tuple(e0, e1, e2);
            }
        };
        template<> struct ToTupleImpl<4> {
            template<typename S> auto operator()(S&& s) const {
                auto[e0, e1, e2, e3] = std::forward<S>(s);
                return std::make_tuple(e0, e1, e2, e3);
            }
        };
        template<> struct ToTupleImpl<5> {
            template<typename S> auto operator()(S&& s) const {
                auto[e0, e1, e2, e3, e4] = std::forward<S>(s);
                return std::make_tuple(e0, e1, e2, e3, e4);
            }
        };
        template<> struct ToTupleImpl<6> {
            template<typename S> auto operator()(S&& s) const {
                auto[e0, e1, e2, e3, e4, e5] = std::forward<S>(s);
                return std::make_tuple(e0, e1, e2, e3, e4, e5);
            }
        };
        template<> struct ToTupleImpl<7> {
            template<typename S> auto operator()(S&& s) const {
                auto[e0, e1, e2, e3, e4, e5, e6] = std::forward<S>(s);
                return std::make_tuple(e0, e1, e2, e3, e4, e5, e6);
            }
        };

        template<std::size_t N, class S>
        auto toTuple(S&& s) {
            return ToTupleImpl<N>{}(std::forward<S>(s));
        }
*/
        template <typename T, typename Head, typename... Tail> struct contains :
        std::disjunction<std::is_same<T, Head>, contains<T, Tail...>> {};
        template <typename T, typename Head> struct contains<T, Head> :
        std::is_same<T, Head> {};

        template <typename T, typename Head, typename... Tail> struct IsConvertibleToSomethingIn_impl {
        private: using _isConvertible = std::disjunction<std::is_convertible<T, Head>, std::is_constructible<Head, T>>;
        public:
            static constexpr bool value = std::disjunction<_isConvertible, IsConvertibleToSomethingIn_impl<T, Tail...>>::value;
            using type = typename std::conditional<contains<T, Head, Tail...>::value,
                                        T,
                                        typename std::conditional<_isConvertible::value,
                                            Head,
                                            typename IsConvertibleToSomethingIn_impl<T, Tail...>::type>::type>::type;
        };

        template <typename T, typename Head> struct IsConvertibleToSomethingIn_impl<T, Head> {
            static constexpr bool value = std::disjunction<std::is_convertible<T, Head>, std::is_constructible<Head, T>>::value;
            using type = typename std::conditional<std::disjunction<std::is_convertible<T, Head>, std::is_constructible<Head, T>>::value, Head, void>::type;
        };

        template <typename T, typename Head, typename... Tail> struct IsConvertibleFromSomethingIn_impl {
        private: using _isConvertible = std::disjunction<std::is_convertible<Head, T>, std::is_constructible<T, Head>>;
        public:
            static constexpr bool value = std::disjunction<_isConvertible, IsConvertibleFromSomethingIn_impl<T, Tail...>>::value;
            using type = typename std::conditional<contains<T, Head, Tail...>::value,
                                        T,
                                        typename std::conditional<_isConvertible::value,
                                                Head,
                                                typename IsConvertibleFromSomethingIn_impl<T, Tail...>::type>::type>::type;
        };

        template <typename T, typename Head> struct IsConvertibleFromSomethingIn_impl<T, Head> {
            static constexpr bool value = std::disjunction<std::is_convertible<Head, T>, std::is_constructible<T, Head>>::value;
            using type = typename std::conditional<value, Head, void>::type;
        };

        template <typename T, typename TUPLE> struct IsConvertibleToSomethingIn;
        template <typename T, typename... TYPES> struct IsConvertibleToSomethingIn<T, std::tuple<TYPES...>> :
        IsConvertibleToSomethingIn_impl<T, TYPES...> {};

        template <typename T, typename TUPLE> struct IsConvertibleFromSomethingIn;
        template <typename T, typename... TYPES> struct IsConvertibleFromSomethingIn<T, std::tuple<TYPES...>> :
        IsConvertibleFromSomethingIn_impl<T, TYPES...> {};

        template <typename ID, typename Head, typename... Tail> struct GetAssociatedDataImpl {
            using type = typename std::conditional<std::is_same<ID, typename Head::first_type>::value,
                                typename Head::second_type,
                                typename GetAssociatedDataImpl<ID, Tail...>::type>::type;
        };

        template <typename ID, typename Head> struct GetAssociatedDataImpl<ID, Head> {
            using type = typename std::conditional<std::is_same<ID, typename Head::first_type>::value,
                                typename Head::second_type,
                                void>::type;
        };

        template <typename ID, typename TUPLE> struct GetAssociatedData;
        template <typename ID, typename... PAIRS> struct GetAssociatedData<ID, std::tuple<PAIRS...>> :
        GetAssociatedDataImpl<ID, PAIRS...> {};

        template <typename T>
        struct Parse {
            static std::optional<T> work(std::string_view s);
        };

        template <>
        struct Parse<std::string> {
            static std::optional<std::string> work(std::string_view s){
                s = trim(s);
                if (s[0] == '"'){
                    auto found = std::find(s.begin()+1, s.end(), '"');
                    if (found == s.end()-1) return {{std::string{s.begin()+1, s.end()-1}}};
                }
                return std::nullopt;
            }
        };

        template <>
        struct Parse<double> {
            static std::optional<double> work(std::string_view s){
                s = trim(s);
                auto str = std::string(s);
                char* end = nullptr;
                double d = std::strtod(str.c_str(), &end);
                if (end == &str[0] + str.length()) return {d};
                return std::nullopt;
            }
        };

        template <>
        struct Parse<bool> {
            static std::optional<bool> work(std::string_view s){
                s = trim(s);
                auto upper = std::string(s);
                std::transform(upper.begin(), upper.end(), upper.begin(), [](auto c){return toupper(c);});
                if (upper == "TRUE") return {{true}};
                else if (upper == "FALSE") return {{false}};
                return std::nullopt;
            }
        };

        template <typename T> void write(std::stringstream& ss, const T& t) { ss << t; }
        template <> inline void write<std::string>(std::stringstream& ss, const std::string& str){ ss << '"' << str << '"'; }

        template <size_t I, typename TUPLE>
        struct StringConverter {
            static void toString(std::stringstream& ss, const TUPLE& data){
                StringConverter<I-1, TUPLE>::toString(ss, data);
                ss <<  ", ";
                write(ss, std::get<I>(data));
            }

            static bool parse(std::string_view sv, TUPLE& data){
                sv = trim(sv);
                int commaIndex = sv.rfind(',');
                auto psv = sv.substr(commaIndex + 1, sv.size() - commaIndex);
                auto value = Parse<typename std::tuple_element<I, TUPLE>::type>::work(psv);
                if (value){
                    std::get<I>(data) = value.value();
                    return StringConverter<I-1, TUPLE>::parse(sv.substr(0, commaIndex), data);
                }
                return false;
            }
        };

        template <typename TUPLE>
        struct StringConverter<0, TUPLE> {
            static void toString(std::stringstream& ss, const TUPLE& data){
                write(ss, std::get<0>(data));
            }

            static bool parse(std::string_view sv, TUPLE& data){
                sv = trim(sv);
                auto value = Parse<typename std::tuple_element<0, TUPLE>::type>::work(sv);
                if (value){
                    std::get<0>(data) = value.value();
                    return true;
                }
                return false;
            }
        };
    }

    // Generates the global accelerating structures used during the parsing process for an enum type.
    // Must be called before using the library with that enum.
    //
    //      First template parameter is the enum type to be configured.
    //      Second template parameter is the number of different values in the enum.
    //      The enum values must cover the full range of [0, SIZE].
    //
    //      First parameter is a function with the signature: std::string(ENUM)
    //      The function must return the string associated with the enum value.
    template <typename ENUM, ENUM SIZE = ENUM::_SIZE, typename F>
    inline void configure(const F& getName) {
        for (size_t i = 0; i < static_cast<size_t>(SIZE); ++i){
            auto type = static_cast<ENUM>(i);
            auto name = getName(type);
            detail::getNameToType<ENUM, SIZE>()[name] = type;
            detail::getTypeToName<ENUM, SIZE>()[i] = name;
        }
    }

    namespace detail {
        enum class DefaultGroup { GENERAL, _SIZE };

        static auto _dummy = [](){
            configure<DefaultGroup>([](auto g){ return "general"; });
            return nullptr;
        }();
    }

    // Helper struct to use typed enums for configuration values as well
    template <typename ENUM, ENUM SIZE = ENUM::_SIZE>
    struct EnumeratedData final {
        ENUM value;

        EnumeratedData() = default;
        EnumeratedData(ENUM value) : value{value} {}
        operator ENUM() const noexcept { return value; }

        std::string toString() const {
            return detail::getTypeToName<ENUM, SIZE>()[static_cast<size_t>(value)];
        }

        static std::optional<EnumeratedData> parse(std::string_view s) {
            auto& map = detail::getNameToType<ENUM, SIZE>();
            auto found = map.find(std::string(s));
            if (found == map.end()) return std::nullopt;
            else return std::make_optional(EnumeratedData{found->second});
        }
    };

    template <typename T, typename = void> struct ConvertEnumToEnumeratedData {};

    template <typename T> struct ConvertEnumToEnumeratedData<T, typename std::enable_if<std::is_enum_v<T>>::type> {
        using type = EnumeratedData<T>;
    };

    template <typename T> struct ConvertEnumToEnumeratedData<T, typename std::enable_if<!std::is_enum_v<T>>::type> {
        using type = T;
    };

    template <typename ENUM, ENUM SIZE = ENUM::_SIZE>
    EnumeratedData<ENUM, SIZE> make_enumerated_data(ENUM arg) {
        return EnumeratedData<ENUM, SIZE>{arg};
    }

    template <typename ENUM, ENUM SIZE = ENUM::_SIZE>
    std::ostream& operator<<(std::ostream& os, const EnumeratedData<ENUM, SIZE>& data){
        os << data.toString();
        return os;
    }

    namespace detail {
        template <typename ENUM, ENUM SIZE>
        struct Parse<EnumeratedData<ENUM, SIZE>> {
            static std::optional<EnumeratedData<ENUM, SIZE>> work(std::string_view s){
                s = trim(s);
                auto data = EnumeratedData<ENUM, SIZE>::parse(s);
                if (data) return data;
                return std::nullopt;
            }
        };
    }

    // Helper struct to easily compose complex data structures for configuration values
    template <typename... T>
    struct ComplexData final {
        std::tuple<T...> values;

        ComplexData() = default;
        ComplexData(T... args) : values{std::move(args)...} {}
  //      template <typename S, typename = std::void_t<decltype(detail::toTuple<3>(*(S*)(nullptr)))>> ComplexData(S&& valueStruct) : values{detail::toTuple<3>(valueStruct)} {}

        template <typename R>
        operator R() const { return detail::ToStruct<R>(values); }

        std::string toString() const {
            std::stringstream ss;
            ss << std::boolalpha << "{ ";
            detail::StringConverter<std::tuple_size<std::tuple<T...>>::value-1, std::tuple<T...>>::toString(ss, values);
            ss << " }";
            return ss.str();
        }

        static std::optional<ComplexData> parse(std::string_view s) {
            ComplexData<T...> data;
            if (s.front() != '{' || s.back() != '}') return std::nullopt;
            s = s.substr(1, s.size()-2);
            bool valid = detail::StringConverter<std::tuple_size<std::tuple<T...>>::value-1, std::tuple<T...>>::parse(s, data.values);
            if (valid) return {data};
            return std::nullopt;
        }
    };

    template <typename... T>
    ComplexData<typename ConvertEnumToEnumeratedData<T>::type...> make_complex_data(T&&... args) {
        return ComplexData<typename ConvertEnumToEnumeratedData<T>::type...>{std::forward<T>(args)...};
    }

    template <typename... T>
    std::ostream& operator<<(std::ostream& os, const ComplexData<T...>& data){
        os << data.toString();
        return os;
    }

    namespace detail {
        template <typename... T>
        struct Parse<ComplexData<T...>> {
            static std::optional<ComplexData<T...>> work(std::string_view s) {
                s = trim(s);
                auto data = ComplexData<T...>::parse(s);
                if (data) return data;
                return std::nullopt;
            }
        };
    }

    // Default data storage class, supports strings, doubles and booleans
    struct DefaultConfigData final {
        std::variant<std::string, double, bool> value;
        using AvailableValueTypes = std::tuple<std::string, double, bool>;

        std::string toString() const {
            switch (value.index()){
                case 0: return '"' + std::get<std::string>(value) + '"';
                case 1: return std::to_string(std::get<double>(value));
                case 2: return std::get<bool>(value) ? "true" : "false";
                default: assert(false); return "";
            }
        }

        static std::optional<DefaultConfigData> parse(const std::string& s){
            auto os = detail::Parse<std::string>::work(s);
            if (os) return {{os.value()}};

            auto ob = detail::Parse<bool>::work(s);
            if (ob) return {{ob.value()}};

            auto od = detail::Parse<double>::work(s);
            if (od) return {{od.value()}};

            return std::nullopt;
        }

        template <typename T> const T& getValueField() const noexcept { assert(std::holds_alternative<T>(value)); return std::get<T>(value); }
        template <typename T> void setValueField(T&& t) { value = std::forward<T>(t); }
    };

    namespace detail {
        // Represents a collection of grouped key-value pairs.
        // The template parameters defines the enums used to access the values, and stored data type.
        template <typename ENUM, typename DATA = DefaultConfigData, size_t ENUM_SIZE = (size_t)ENUM::_SIZE>
        class EnumMap final {
        public:
            using enum_type = ENUM;
            using data_type = DATA;
            using storage_type = typename std::aligned_storage<sizeof(DATA), alignof(DATA)>::type;

            EnumMap() = default;

            EnumMap(const EnumMap& that) noexcept(std::is_nothrow_copy_constructible_v<DATA>) {
                for (int i = 0; i < ENUM_SIZE; ++i) {
                    if (that.isDataValid((ENUM)i)) {
                        setData((ENUM)i, that.data((ENUM)i));
                    }
                }
            }

            EnumMap(EnumMap&& that) noexcept(std::is_nothrow_move_constructible_v<DATA>) {
                for (int i = 0; i < ENUM_SIZE; ++i) {
                    if (that.isDataValid((ENUM)i)) {
                        setData((ENUM)i, std::move(that.data((ENUM)i)));
                    }
                }
            }

            EnumMap& operator=(EnumMap that) noexcept(std::is_nothrow_move_assignable_v<DATA> && std::is_nothrow_move_constructible_v<DATA>) {
                for (int i = 0; i < ENUM_SIZE; ++i) {
                    if (that.isDataValid((ENUM)i)) {
                        setData((ENUM)i, std::move(that.data((ENUM)i)));
                    } else {
                        clear((ENUM)i);
                    }
                    valids[i] = that.valids[i];
                }
                return *this;
            }

            ~EnumMap() noexcept {
                clear();
            }

            bool isDataValid(ENUM id) const noexcept {
                return valids[static_cast<size_t>(id)] & 1u;
            }

            bool shouldSaveData(ENUM id) const noexcept {
                return  isDataValid(id) && valids[static_cast<size_t>(id)] & 2u;
            }

            void markDataForSave(ENUM id) noexcept {
                assert(isDataValid(id));
                valids[static_cast<size_t>(id)] |=  2u;
            }

            void unMarkDataForSave(ENUM id) noexcept {
                assert(isDataValid(id));
                valids[static_cast<size_t>(id)] &=  ~2u;
            }

            void setData(ENUM id, DATA&& data) noexcept(std::is_nothrow_move_constructible_v<DATA>) {
                if (isDataValid(id)) {
                    *std::launder(reinterpret_cast<DATA*>(&values[static_cast<size_t>(id)])) = std::move(data);
                } else {
                    new(&values[static_cast<size_t>(id)]) DATA{std::move(data)};
                }
                valids[static_cast<size_t>(id)] |= 1u;
            }

            void setData(ENUM id, const DATA& data) noexcept(std::is_nothrow_copy_constructible_v<DATA>) {
                if (isDataValid(id)) {
                    *std::launder(reinterpret_cast<DATA*>(&values[static_cast<size_t>(id)])) = data;
                } else {
                    new(&values[static_cast<size_t>(id)]) DATA{data};
                }
                valids[static_cast<size_t>(id)] |= 1u;
            }

            DATA& data(ENUM id) noexcept {
                assert(isDataValid(id));
                return *std::launder(reinterpret_cast<DATA*>(&values[static_cast<size_t>(id)]));
            }

            const DATA& data(ENUM id) const noexcept {
                assert(isDataValid(id));
                return *std::launder(reinterpret_cast<DATA*>(&values[static_cast<size_t>(id)]));
            }

            void clear(ENUM id) noexcept {
                if (isDataValid(id)) {
                    valids[static_cast<size_t>(id)] = 0;
                    std::launder(reinterpret_cast<DATA*>(&values[static_cast<size_t>(id)]))->~DATA();
                }
            }

            void clear() noexcept {
                for (int i = 0; i < ENUM_SIZE; ++i) {
                    clear((ENUM)i);
                }
            }

        private:
            std::array<storage_type, static_cast<size_t>(ENUM_SIZE)> values;
            std::array<unsigned char, static_cast<size_t>(ENUM_SIZE)> valids;
        };

        template <typename GROUP, typename... ID_DATA_PAIR>
        class ConfigImpl final {
        private:
            static constexpr size_t GROUP_SIZE = (size_t)GROUP::_SIZE;
            template <typename ID> static constexpr size_t ID_SIZE = (size_t)ID::_SIZE;
            using IdDataList = std::tuple<ID_DATA_PAIR...>;
            template <typename ID> using DATA = typename GetAssociatedData<ID, IdDataList>::type;
            template <typename ID> using IdHolder = EnumMap<ID, DATA<ID>>;
            using IdHolderVariant = std::variant<IdHolder<typename ID_DATA_PAIR::first_type>*...>;
            using GroupHolder = std::array<IdHolderVariant, GROUP_SIZE>;
            template <typename ID, typename T> using isConvertibleFrom = IsConvertibleFromSomethingIn<T, typename DATA<ID>::AvailableValueTypes>;
            template <typename ID, typename T> using isConvertibleTo = IsConvertibleToSomethingIn<T, typename DATA<ID>::AvailableValueTypes>;

        private:
            template <typename ID>
            class ConfigValueProxy final {
            public:
                explicit ConfigValueProxy(ConfigImpl& config, GROUP group, ID id) noexcept
                        : config{config}, group{group}, id{id} {}
                template <typename T> operator T() const noexcept(
#ifdef _MSC_VER
                noexcept(std::declval<ConfigImpl>().get<T>(std::declval<GROUP>(), std::declval<ID>())))
#else
                noexcept(std::declval<ConfigImpl>().template get<T>(std::declval<GROUP>(), std::declval<ID>())))
#endif
                { return config.get<T>(group, id); }

                template <typename T> ConfigValueProxy& operator=(T&& t) noexcept(
#ifdef _MSC_VER
                noexcept(std::declval<ConfigImpl>().set<T>(std::declval<GROUP>(), std::declval<ID>(), std::forward<T>(std::declval<T&&>()))))
#else
                noexcept(std::declval<ConfigImpl>().template set<T>(std::declval<GROUP>(), std::declval<ID>(), std::forward<T>(std::declval<T&&>()))))
#endif
                {
                    config.set(group, id, std::forward<T>(t));
                    return *this;
                }

                template <typename T> T as() /* TODO: noexcept */ {
                    return config.get<T>(group, id);
                }

                template <typename T> bool operator==(const T& t) const {
                    return config.get<T>(group, id) == t;
                }
                template <typename T> bool operator!=(const T& t) const {
                    return config.get<T>(group, id) != t;
                }
                template <typename T> bool operator>=(const T& t) const {
                    return config.get<T>(group, id) >= t;
                }
                template <typename T> bool operator<=(const T& t) const {
                    return config.get<T>(group, id) <= t;
                }
                template <typename T> bool operator<(const T& t) const {
                    return config.get<T>(group, id) < t;
                }
                template <typename T> bool operator>(const T& t) const {
                    return config.get<T>(group, id) > t;
                }
            private:
                ConfigImpl& config;
                GROUP group;
                ID id;
            };

            friend class ConfigGroupProxy;
            class ConfigGroupProxy final {
            public:
                ConfigGroupProxy(ConfigImpl& config, GROUP group) noexcept : config{config}, group{group} {}
                template <typename ID> ConfigValueProxy<ID> operator[](ID id) const noexcept { return config[std::pair{group, id}]; }
            private:
                ConfigImpl& config;
                GROUP group;
            };

            template <typename TUPLE, size_t I>
            struct Initializer { static void init(ConfigImpl& config) {
                    config.configValues[I] = new IdHolder<typename std::tuple_element_t<I, TUPLE>::first_type>();
                    Initializer<TUPLE, I-1>::init(config);
                }};

            template <typename TUPLE>
            struct Initializer<TUPLE, 0> { static void init(ConfigImpl& config) {
                    config.configValues[0] = new IdHolder<typename std::tuple_element_t<0, TUPLE>::first_type>();
                }};

        public:
            ConfigImpl() {
                Initializer<IdDataList, std::tuple_size_v<IdDataList> - 1>::init(*this);
            }

            explicit ConfigImpl(const std::string& filePath) {
                Initializer<IdDataList, std::tuple_size_v<IdDataList> - 1>::init(*this);
                load(filePath);
            }

            ~ConfigImpl() noexcept {
                for (auto& v : configValues){
                    std::visit([](auto&& enumMap){
                        delete enumMap;
                    }, v);
                }
            }

            ConfigImpl(const ConfigImpl& that) {
                Initializer<IdDataList, std::tuple_size_v<IdDataList> - 1>::init(*this);
                load(that, true);
            }

            ConfigImpl(ConfigImpl&& that) noexcept {
                for (int i = 0; i < configValues.size(); ++i) {
                    configValues[i] = std::move(that.configValues[i]);
                    that.configValues[i] = {};
                }
            }

            ConfigImpl& operator=(ConfigImpl that) {
                std::swap(configValues, that.configValues);
                return *this;
            }

            ConfigImpl& load(const std::string& filePath, bool overwrite = true) {
                std::ifstream f(filePath);
                std::stringstream error;

                size_t actGroup = 0;
                std::string line;
                int lineNumber = 0;
                while (std::getline(f, line)) {
                    lineNumber++;
                    trim(line);
                    if (line.length() == 0 || line[0] == ';') {
                        continue;
                    } else if (line[0] == '[') {
                        auto found = std::find(line.begin(), line.end(), ']');
                        if (found != line.end()){
                            auto s = line.substr(1, static_cast<unsigned long long>(found - line.begin() - 1));
                            trim(s);
                            auto g = groupFromString(s);
                            if (g) {
                                actGroup = static_cast<size_t>(g.value());
                            } else {
                                error << "Group name - " << s << " is invalid!\n";
                            }
                        } else {
                            error << "Line " << lineNumber << " is invalid: missing ']'!\n";
                        }
                    } else {
                        auto found = std::find(line.begin(), line.end(), '=');
                        if (found != line.end()){
                            auto s = line.substr(0, static_cast<unsigned long long>(found - line.begin()));
                            trim(s);

                            auto& ihv = configValues[actGroup];
                            std::visit([&](auto&& enumMap) {
                                using ID = typename std::decay_t<decltype(*enumMap)>::enum_type;

                                auto n = idFromString<ID>(s);
                                if (n) {
                                    s = std::string{found + 1, line.end()};
                                    trim(s);
                                    if (s.length() == 0) {
                                        error << "Line " << lineNumber << " is invalid: missing value!\n";
                                        return;
                                    }
                                    auto opt = DATA<ID>::parse(s);
                                    if (opt) {
                                        if (overwrite || !isDataValid((GROUP)actGroup, n.value())) {
                                            setData((GROUP)actGroup, n.value(), std::move(opt.value()));
                                            markDataForSave((GROUP)actGroup, n.value());
                                        }
                                    } else {
                                        error << "Line " << lineNumber << " is invalid: value format is not parsable!\n";
                                    }
                                } else {
                                    error << "Configuration name - " << s << " is invalid!\n";
                                }
                            }, ihv);
                        } else {
                            error << "Line " << lineNumber << " is invalid: missing '='!\n";
                        }
                    }
                }
                if (!error.str().empty()) {
                    errorFunction(error.str());
                }
                return *this;
            }

            ConfigImpl& load(const ConfigImpl& config, bool overwrite) {
                for (size_t i = 0; i < static_cast<size_t>(GROUP_SIZE); ++i) {
                    auto& ihv = configValues[i];
                    std::visit([&](auto&& enumMap) {
                        using ID = typename std::decay_t<decltype(*enumMap)>::enum_type;
                        for (size_t j = 0; j < static_cast<size_t>(ID_SIZE<ID>); ++j) {
                            if (config.isDataValid((GROUP)i, (ID)j) && (overwrite || !isDataValid((GROUP)i, (ID)j))) {
                                setData((GROUP)i, (ID)j, config.data((GROUP)i, (ID)j));
                                if (config.shouldSaveData((GROUP)i, (ID)j)) markDataForSave((GROUP)i, (ID)j);
                            }
                        }
                    }, ihv);
                }
                return *this;
            }

            ConfigImpl& load(ConfigImpl&& config, bool overwrite) {
                for (size_t i = 0; i < static_cast<size_t>(GROUP_SIZE); ++i) {
                    auto& ihv = configValues[i];
                    std::visit([&](auto&& enumMap) {
                        using ID = typename std::decay_t<decltype(*enumMap)>::enum_type;
                        for (size_t j = 0; j < static_cast<size_t>(ID_SIZE<ID>); ++j) {
                            if (config.isDataValid((GROUP)i, (ID)j) && (overwrite || !isDataValid((GROUP)i, (ID)j))) {
                                setData((GROUP)i, (ID)j, std::move(config.data((GROUP)i, (ID)j)));
                                if (config.shouldSaveData((GROUP)i, (ID)j)) markDataForSave((GROUP)i, (ID)j);
                            }
                        }
                    }, ihv);
                }
                return *this;
            }

            template <typename ID>
            ConfigImpl& commit(GROUP group, ID id) noexcept {
                markDataForSave(group, id);
                return *this;
            }

            template <typename ID>
            ConfigImpl& commit(ID id) noexcept {
                return commit((GROUP)0, id);
            }

            template <typename ID>
            ConfigImpl& forget(GROUP group, ID id) noexcept {
                unMarkDataForSave(group, id);
                return *this;
            }

            template <typename ID>
            ConfigImpl& forget(ID id) noexcept {
                return forget((GROUP)0, id);
            }

            bool save(const std::string& filePath) const {
                std::ofstream f(filePath);
                if (!f) return false;
                f << std::boolalpha;

                for (size_t i = 0; i < static_cast<size_t>(GROUP_SIZE); ++i) {
                    f << '[' << toString(static_cast<GROUP>(i)) << ']' << '\n';
                    auto& ihv = configValues[i];
                    std::visit([&](auto&& enumMap) {
                        using ID = typename std::decay_t<decltype(*enumMap)>::enum_type;
                        for (size_t j = 0; j < static_cast<size_t>(ID_SIZE<ID>); ++j) {
                            if (shouldSaveData((GROUP)i, (ID)j)) {
                                f << toString(static_cast<ID>(j)) << "=" <<  data((GROUP)i, (ID)j).toString() << '\n';
                            }
                        }
                        f << '\n';
                    }, ihv);
                }
                return !!f;
            }

            bool filled() const noexcept {
                bool valid = true;
                for (size_t i = 0; i < static_cast<size_t>(GROUP_SIZE) && valid; ++i){
                    auto& ihv = configValues[i];
                    std::visit([&](auto&& enumMap) {
                        using ID = typename std::decay_t<decltype(*enumMap)>::enum_type;
                        for (size_t j = 0; j < static_cast<size_t>(ID_SIZE<ID>); ++j) {
                            if (!isDataValid((GROUP)i, (ID)j)) {
                                valid = false;
                                return;
                            }
                        }
                    }, ihv);
                }
                return valid;
            }

            void setErrorFunction(std::function<void(const std::string&)> errorHandler) {
                errorFunction = std::move(errorHandler);
            }

            template <typename T, typename ID>
            T get(GROUP group, ID id) const noexcept(noexcept(T(std::declval<typename isConvertibleFrom<ID, T>::type>()))) {
                static_assert(isConvertibleFrom<ID, T>::value, "incompatible types");
#ifdef _MSC_VER
                return T(data(group, id).getValueField<typename isConvertibleFrom<ID, T>::type>());
#else
                return T(data(group, id).template getValueField<typename isConvertibleFrom<ID, T>::type>());
#endif
            }

            template <typename T, typename ID>
            T get(ID id) const noexcept(noexcept(T(std::declval<typename isConvertibleFrom<ID, T>::type>()))) {
                return get<T>(static_cast<GROUP>(0), id);
            }

            template <typename ID>
            ConfigValueProxy<ID> operator[](std::pair<GROUP, ID> idx) noexcept {
                return ConfigValueProxy<ID>{*this, idx.first, idx.second};
            }

            ConfigGroupProxy operator[](GROUP group) noexcept {
                return ConfigGroupProxy{*this, group};
            }

            template <typename ID>
            ConfigValueProxy<ID> operator[](ID id) noexcept {
                return ConfigValueProxy<ID>{*this, static_cast<GROUP>(0), id};
            }

            template <typename T, typename ID>
            void set(GROUP group, ID id, T&& t) noexcept(noexcept(std::declval<ConfigImpl>().setData(std::declval<GROUP>(), std::declval<ID>(), DATA<ID>{(typename isConvertibleTo<ID, T>::type)std::forward<T>(std::declval<T&&>())}))) {
                static_assert(isConvertibleTo<ID, T>::value, "incompatible types");
                setData(group, id, DATA<ID>{(typename isConvertibleTo<ID, T>::type)std::forward<T>(t)});
            }

            template <typename T, typename ID>
            void set(ID id, T&& t) noexcept(noexcept(std::declval<ConfigImpl>().set(std::declval<GROUP>(), std::declval<ID>(), std::forward<T>(std::declval<T&&>())))) {
                set(static_cast<GROUP>(0), id, std::forward<T>(t));
            }

            template <typename T, typename ID>
            void setIfEmpty(GROUP group, ID id, T&& t) noexcept(noexcept(std::declval<ConfigImpl>().set(std::declval<GROUP>(), std::declval<ID>(), std::forward<T>(std::declval<T&&>())))) {
                static_assert(isConvertibleTo<ID, T>::value, "incompatible types");
                if (!isDataValid(group, id)) {
                    set(group, id, std::forward<T>(t));
                }
            }

            template <typename T, typename ID>
            void setIfEmpty(ID id, T&& t) noexcept(noexcept(std::declval<ConfigImpl>().setIfEmpty(std::declval<GROUP>(), std::declval<ID>()))) {
                setIfEmpty(static_cast<GROUP>(0), id, std::forward<T>(t));
            }

        private: // Helper functions
            std::optional<GROUP> groupFromString(const std::string& str) noexcept {
                auto& map = getNameToType<GROUP, GROUP::_SIZE>();
                auto found = map.find(str);
                if (found == map.end()) return std::nullopt;
                return std::make_optional(found->second);
            }

            template <typename ID>
            std::optional<ID> idFromString(const std::string& str) noexcept {
                auto& map = getNameToType<ID, ID::_SIZE>();
                auto found = map.find(str);
                if (found == map.end()) return std::nullopt;
                return std::make_optional(found->second);
            }

            std::string toString(GROUP group) const {
                return getTypeToName<GROUP, (GROUP)GROUP_SIZE>()[static_cast<size_t>(group)];
            }

            template <typename ID>
            std::string toString(ID id) const {
                return getTypeToName<ID>()[static_cast<size_t>(id)];
            }

            template <typename ID>
            void setData(GROUP group, ID id, DATA<ID>&& data) noexcept(std::is_nothrow_move_constructible_v<DATA<ID>>) {
                std::get<IdHolder<ID>*>(configValues[(size_t)group])->setData(id, std::move(data));
            }

            template <typename ID>
            void setData(GROUP group, ID id, const DATA<ID>& data) noexcept(std::is_nothrow_copy_constructible_v<DATA<ID>>) {
                std::get<IdHolder<ID>*>(configValues[(size_t)group])->setData(id, data);
            }

            template <typename ID>
            void markDataForSave(GROUP group, ID id) noexcept {
                return std::get<IdHolder<ID>*>(configValues[(size_t)group])->markDataForSave(id);
            }

            template <typename ID>
            void unMarkDataForSave(GROUP group, ID id) noexcept {
                return std::get<IdHolder<ID>*>(configValues[(size_t)group])->unMarkDataForSave(id);
            }

            template <typename ID>
            bool shouldSaveData(GROUP group, ID id) const noexcept {
                return std::get<IdHolder<ID>*>(configValues[(size_t)group])->shouldSaveData(id);
            }

            template <typename ID>
            bool isDataValid(GROUP group, ID id) const noexcept {
                return std::get<IdHolder<ID>*>(configValues[(size_t)group])->isDataValid(id);
            }

            template <typename ID>
            DATA<ID>& data(GROUP group, ID id) noexcept {
                return std::get<IdHolder<ID>*>(configValues[(size_t)group])->data(id);
            }

            template <typename ID>
            const DATA<ID>& data(GROUP group, ID id) const noexcept {
                return std::get<IdHolder<ID>*>(configValues[(size_t)group])->data(id);
            }

/*        private: // These functions and classes are probably not useful so I hid them
            template <typename ID, typename T>
            class ConfigBoundVariable final {
            public:
                ConfigBoundVariable(ConfigImpl& config, GROUP group, ID id)
                        : config{config}, group{group}, id{id}
                {}
                operator T() const { return config.get<T>(group, id); }
                void update(const T& t) { config.set<T>(group, id, t); }
            private:
                ConfigImpl& config;
                GROUP group;
                ID id;
            };

            template <typename T, typename ID>
            std::optional<T> tryGet(GROUP group, ID id) const {
                static_assert(isConvertibleFrom<ID, T>::value, "incompatible types");
                auto& opt = getOptionalData(group, id);
                try {
#ifdef _MSC_VER
                    return opt ? std::make_optional(T(opt.value().getValueField<typename isConvertibleFrom<ID, T>::type>())) : std::nullopt;
#else
                    return opt ? std::make_optional(T(opt.value().template getValueField<typename isConvertibleFrom<ID, T>::type>())) : std::nullopt;
#endif
                } catch (...) {
                    throw std::runtime_error("<" + toString(group) + ", " + toString(id) + "> is accessed with bad type!");
                }
            }

            template <typename T, typename ID>
            std::optional<T> tryGet(ID id) const {
                return tryGet<T>(static_cast<GROUP>(0), id);
            }

            template <typename T, typename ID>
            std::optional<T> mustGet(GROUP group, ID id) const noexcept {
                auto& opt = getOptionalData(group, id);
                try {
#ifdef _MSC_VER
                    return opt ? std::make_optional(opt.value().getValueField<T>()) : std::nullopt;
#else
                    return opt ? std::make_optional(opt.value().template getValueField<T>()) : std::nullopt;
#endif
                } catch (...) {
                    return std::nullopt;
                }
            }

            template <typename T, typename ID>
            std::optional<T> mustGet(ID id) const noexcept {
                return mustGet<T>(static_cast<GROUP>(0), id);
            }

            template <typename T, typename ID>
            ConfigBoundVariable<ID, T> bind(GROUP groupName, ID configName) {
                return ConfigBoundVariable<ID, T>{*this, groupName, configName};
            }

            template <typename T, typename ID>
            ConfigBoundVariable<ID, T> bind(ID configName) {
                return bind<T>(static_cast<GROUP>(0), configName);
            }

            template <typename ID>
            void clear(GROUP group, ID id) {
                std::visit([&](auto&& enumMap){
                    enumMap->clear(group, id);
                }, configValues[static_cast<size_t>(group)]);
            }

            void clear() {
                for (auto& v : configValues){
                    std::visit([](auto&& enumMap){
                        enumMap->clear();
                    }, v);
                }
            }

            template <typename ID>
            std::optional<std::reference_wrapper<DATA<ID>>> getData(GROUP group, ID id) noexcept(std::reference_wrapper<DATA<ID>>{std::declval<DATA<ID>>()}) {
                auto& opt = getOptionalData(group, id);
                return opt ? std::make_optional(std::reference_wrapper<DATA<ID>>{opt.value()}) : std::nullopt;
            }

            template <typename ID>
            std::optional<std::reference_wrapper<DATA<ID>>> getData(ID id) noexcept(getData(std::declval<GROUP>(), std::declval<ID>())) {
                return getData(static_cast<GROUP>(0), id);
            }
*/
        private:
            GroupHolder configValues;
            std::function<void(const std::string&)> errorFunction = [](const auto&) {};
        };
    }

    namespace detail {
        template <typename T> struct choose_type {
            using type = std::pair<T, DefaultConfigData>;
        };
        template <typename T1, typename T2> struct choose_type<std::pair<T1, T2>> {
            using type = std::pair<T1, T2>;
        };
        template <typename T> using choose_type_t = typename choose_type<T>::type;

        template <typename Enable, typename T1, typename T2, typename... TS>
        struct ConfigImplTypeSwizzler final { using type = void; };

        // DefaultGroup
        template <typename T1>
        struct ConfigImplTypeSwizzler<void, T1, DefaultGroup> final {
            using type = ConfigImpl<DefaultGroup, choose_type_t<T1>>;
        };

        // CustomGroup
        template <typename T1, typename T2, typename... TS>
        struct ConfigImplTypeSwizzler<void, T1, T2, TS...> final {
            using type = ConfigImpl<T1, choose_type_t<T2>, choose_type_t<TS>...>;
        };
    }

    template <typename T1, typename T2 = detail::DefaultGroup, typename... TS>
    using Config = typename detail::ConfigImplTypeSwizzler<void, T1, T2, TS...>::type;
}

#endif //CPP_CONFIG_CONFIG_HPP