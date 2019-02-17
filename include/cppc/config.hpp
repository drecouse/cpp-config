#ifndef CPP_CONFIG_CONFIG_HPP
#define CPP_CONFIG_CONFIG_HPP

#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include <variant>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <cassert>

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
    inline void configure(const F& getName){
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

        EnumeratedData(ENUM value) : value{value} {}
        operator ENUM() const { return value; }

        std::string toString() const {
            return detail::getTypeToName<ENUM, SIZE>()[static_cast<size_t>(value)];
        }

        static std::optional<EnumeratedData> parse(const std::string& s) {
            auto& map = detail::getNameToType<ENUM, SIZE>();
            auto found = map.find(s);
            if (found == map.end()) return std::nullopt;
            else return std::make_optional(EnumeratedData{found->second});
        }

        template <typename T, typename = void> T getValueField() const;
        template <typename = void> ENUM getValueField() const { return value; }
        template <typename T, typename = void> void setValueField(const T& e);
        template <typename = void> void setValueField(const ENUM& e){ value = e; }
    };

    // Default data storage class, supports strings, doubles and booleans
    struct ConfigData final {
        std::variant<std::string, double, bool> value;
        using AvailableValueTypes = std::tuple<std::string, double, bool>;

        std::string toString() const {
            switch (value.index()){
                case 0: return std::get<std::string>(value);
                case 1: return std::to_string(std::get<double>(value));
                case 2: return std::to_string(std::get<bool>(value));
                default: assert(false); return "";
            }
        }

        static std::optional<ConfigData> parse(const std::string& s){
            if (s[0] == '"'){
                auto found = std::find(s.begin()+1, s.end(), '"');
                if (found == s.end()-1) return {{std::string{s.begin()+1, s.end()-1}}};
                else return std::nullopt;
            }

            auto upper = s;
            std::transform(upper.begin(), upper.end(), upper.begin(), [](auto c){return toupper(c);});
            if (upper == "TRUE") return {{true}};
            else if (upper == "FALSE") return {{false}};

            char* end = nullptr;
            double d = std::strtod(s.c_str(), &end);
            if (end == &s[0] + s.length()) return {{d}};

            return std::nullopt;
        }

        template <typename T> T getValueField() const { return std::get<T>(value); }
        template <typename T> void setValueField(const T& t){ value = t; }
    };

    // Represents a collection of grouped key-value pairs.
    // The template parameters defines the enums used to access the values, and stored data type.
    template <typename GROUP, typename NAME, typename DATA = ConfigData, GROUP GROUP_SIZE = GROUP::_SIZE, NAME NAME_SIZE = NAME::_SIZE>
    class ConfigImpl final {
    public:
        using GroupName = GROUP;
        using ConfigName = NAME;
        using DataType = DATA;

    private:
        template <typename T>
        class ConfigBoundVariable final {
        public:
            ConfigBoundVariable(ConfigImpl& config, typename ConfigImpl::GroupName groupName, typename ConfigImpl::ConfigName configName)
                    : config{config}
                    , groupName{groupName}
                    , configName{configName}
            {}
            operator T() const { return config.get<T>(groupName, configName); }
            void update(const T& t) { config.set<T>(groupName, configName, t); }
        private:
            ConfigImpl& config;
            typename ConfigImpl::GroupName groupName;
            typename ConfigImpl::ConfigName configName;
        };

        class ConfigValueProxy final {
        public:
            explicit ConfigValueProxy(ConfigImpl& config, GroupName groupName, ConfigName configName)
                    : config{config}, groupName{groupName}, configName{configName} {}
            template <typename T> operator T() const { return config.get<T>(groupName, configName); }
            template <typename T> ConfigValueProxy& operator=(T&& t) {
                config.set<T>(groupName, configName, std::forward<T>(t));
                return *this;
            }
        private:
            ConfigImpl& config;
            GroupName groupName;
            ConfigName configName;
        };

        friend class ConfigGroupProxy;
        class ConfigGroupProxy final {
        public:
            ConfigGroupProxy(ConfigImpl& config, GroupName groupName) : config{config}, groupName{groupName} {}
            ConfigValueProxy operator[](ConfigName configName) { return config[{groupName, configName}]; }
        private:
            ConfigImpl& config;
            GroupName groupName;
        };

        template <typename T> using isConvertibleFrom = detail::IsConvertibleFromSomethingIn<T, typename DataType::AvailableValueTypes>;
        template <typename T> using isConvertibleTo = detail::IsConvertibleToSomethingIn<T, typename DataType::AvailableValueTypes>;

    public:
        explicit ConfigImpl(const std::string& filePath) {
            load(filePath);
        }

        ConfigImpl& load(const std::string& filePath, bool overwrite = true){
            std::ifstream f(filePath);

            size_t actGroup = 0;
            std::string line;
            int lineNumber = 0;
            while (std::getline(f, line)){
                lineNumber++;
                detail::trim(line);
                if (line.length() == 0 || line[0] == ';'){
                    continue;
                }
                else if (line[0] == '['){
                    auto found = std::find(line.begin(), line.end(), ']');
                    if (found != line.end()){
                        auto s = line.substr(1, static_cast<unsigned long long>(found - line.begin() - 1));
                        detail::trim(s);
                        auto g = configGroupFromString(s);
                        if (g){
                            actGroup = static_cast<size_t>(g.value());
                        } else {
                            std::cerr << "Group name - " << s << " is invalid!\n";
                        }
                    } else {
                        std::cerr << "Line " << lineNumber << " is invalid: missing ']'!\n";
                    }
                } else {
                    auto found = std::find(line.begin(), line.end(), '=');
                    if (found != line.end()){
                        auto s = line.substr(0, static_cast<unsigned long long>(found - line.begin()));
                        detail::trim(s);
                        auto n = configNameFromString(s);
                        if (n){
                            s = std::string{found + 1, line.end()};
                            detail::trim(s);
                            if (s.length() == 0){
                                std::cerr << "Line " << lineNumber << " is invalid: missing value!\n";
                                continue;
                            }
                            auto opt = DATA::parse(s);
                            if (opt){
                                if (overwrite || !getOptionalData(static_cast<GroupName>(actGroup), n.value())){
                                    getOptionalData(static_cast<GroupName>(actGroup), n.value()) = opt;
                                }
                            } else {
                                std::cerr << "Line " << lineNumber << " is invalid: value format is not parsable!\n";
                            }
                        } else {
                            std::cerr << "Configuration name - " << s << " is invalid!\n";
                        }
                    } else {
                        std::cerr << "Line " << lineNumber << " is invalid: missing '='!\n";
                    }
                }
            }
            return *this;
        }

        ConfigImpl& load(const ConfigImpl& config, bool overwrite) {
            for (size_t i = 0; i < static_cast<size_t>(GROUP_SIZE); ++i){
                for (size_t j = 0; j < static_cast<size_t>(NAME_SIZE); ++j){
                    if (config.configValues[i][j] && (overwrite || !configValues[i][j])){
                        configValues[i][j] = config.configValues[i][j];
                    }
                }
            }
            return *this;
        }

        bool save(const std::string& filePath) const {
            std::ofstream f(filePath);
            if (!f) return false;

            for (size_t i = 0; i < static_cast<size_t>(GROUP_SIZE); ++i){
                f << '[' << toString(static_cast<GroupName >(i)) << ']' << '\n';
                for (size_t j = 0; j < static_cast<size_t>(NAME_SIZE); ++j){
                    if (configValues[i][j]){
                        f << toString(static_cast<ConfigName >(j)) << "=" << configValues[i][j].value().toString() << '\n';
                    }
                }
                f << '\n';
            }
            return !!f;
        }

        void clear(GroupName groupName, ConfigName configName) {
            getOptionalData(groupName, configName).reset();
        }

        void clear() {
            for (auto& group : configValues) {
                for (auto& config : group){
                    config.reset();
                }
            }
        }

        template <typename T>
        ConfigBoundVariable<T> bind(GroupName groupName, ConfigName configName) {
            return ConfigBoundVariable<T>{*this, groupName, configName};
        }

        template <typename T>
        ConfigBoundVariable<T> bind(ConfigName configName) {
            return bind<T>(static_cast<GroupName>(0), configName);
        }

        template <typename T>
        T get(GroupName groupName, ConfigName configName) const {
            static_assert(isConvertibleFrom<T>::value, "incompatible types");
            try {
#ifdef _MSC_VER
                return T(getOptionalData(groupName, configName).value().getValueField<typename isConvertibleFrom<T>::type>());
#else
                return T(getOptionalData(groupName, configName).value().template getValueField<typename isConvertibleFrom<T>::type>());
#endif
            } catch (const std::bad_optional_access&) {
                throw std::runtime_error("<" + toString(groupName) + ", " + toString(configName) + "> is not found!");
            } catch (...) {
                throw std::runtime_error("<" + toString(groupName) + ", " + toString(configName) + "> is accessed with bad type!");
            }
        }

        template <typename T>
        T get(ConfigName configName) const {
            return get<T>(static_cast<GroupName>(0), configName);
        }

        template <typename T>
        std::optional<T> tryGet(GroupName groupName, ConfigName configName) const {
            static_assert(isConvertibleFrom<T>::value, "incompatible types");
            auto& opt = getOptionalData(groupName, configName);
            try {
#ifdef _MSC_VER
                return opt ? std::make_optional(T(opt.value().getValueField<typename isConvertibleFrom<T>::type>())) : std::nullopt;
#else
                return opt ? std::make_optional(T(opt.value().template getValueField<typename isConvertibleFrom<T>::type>())) : std::nullopt;
#endif
            } catch (...) {
                throw std::runtime_error("<" + toString(groupName) + ", " + toString(configName) + "> is accessed with bad type!");
            }
        }

        template <typename T>
        std::optional<T> tryGet(ConfigName configName) const {
            return tryGet<T>(static_cast<GroupName>(0), configName);
        }

        template <typename T>
        std::optional<T> mustGet(GroupName groupName, ConfigName configName) const noexcept {
            auto& opt = getOptionalData(groupName, configName);
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

        template <typename T>
        std::optional<T> mustGet(ConfigName configName) const noexcept {
            return mustGet<T>(static_cast<GroupName>(0), configName);
        }

        ConfigValueProxy operator[](std::pair<GroupName, ConfigName> idx) {
            return ConfigValueProxy{*this, idx.first, idx.second};
        }

        ConfigGroupProxy operator[](GroupName groupName) {
            return ConfigGroupProxy{*this, groupName};
        }

        ConfigValueProxy operator[](ConfigName configName) {
            return ConfigValueProxy{*this, static_cast<GroupName>(0), configName};
        }

        template <typename T>
        void set(GroupName groupName, ConfigName configName, const T& t) {
            static_assert(isConvertibleTo<T>::value, "incompatible types");
            auto& opt = getOptionalData(groupName, configName);
            if (!opt){
                opt = DataType{(typename isConvertibleTo<T>::type)t};
            } else {
                opt.value().setValueField((typename isConvertibleTo<T>::type)t);
            }
        }

        template <typename T>
        void set(ConfigName configName, const T& t) {
            set<T>(static_cast<GroupName>(0), configName, t);
        }

        std::optional<std::reference_wrapper<DATA>> getData(GroupName groupName, ConfigName configName) {
            auto& opt = getOptionalData(groupName, configName);;
            return opt ? std::make_optional(std::reference_wrapper<DATA>{opt.value()}) : std::nullopt;
        }

        std::optional<std::reference_wrapper<DATA>> getData(ConfigName configName) {
            return getData(static_cast<GroupName>(0), configName);
        }

    private:
        std::string toString(ConfigName name) const {
            return detail::getTypeToName<ConfigName, NAME_SIZE>()[static_cast<size_t>(name)];
        }

        std::string toString(GroupName group) const {
            return detail::getTypeToName<GroupName, GROUP_SIZE>()[static_cast<size_t>(group)];
        }

        std::optional<ConfigName> configNameFromString(const std::string& str) const {
            auto& map = detail::getNameToType<NAME, NAME_SIZE>();
            auto found = map.find(str);
            if (found == map.end()) return std::nullopt;
            return std::make_optional(found->second);
        }

        std::optional<GroupName> configGroupFromString(const std::string& str) const {
            auto& map = detail::getNameToType<GROUP, GROUP_SIZE>();
            auto found = map.find(str);
            if (found == map.end()) return std::nullopt;
            return std::make_optional(found->second);
        }

        std::optional<DataType>& getOptionalData(GroupName groupName, ConfigName configName) {
            return configValues[static_cast<size_t>(groupName)][static_cast<size_t>(configName)];
        }

        const std::optional<DataType>& getOptionalData(GroupName groupName, ConfigName configName) const {
            return configValues[static_cast<size_t>(groupName)][static_cast<size_t>(configName)];
        }

    private:
        std::array<std::array<std::optional<DataType>, static_cast<size_t>(NAME_SIZE)>, static_cast<size_t>(GROUP_SIZE)> configValues;
    };

    namespace detail {
        template <typename T1, typename T2, typename T3, typename Enable = void>
        struct ConfigImplTypeSwizzler final {};

        template <typename T1>
        struct ConfigImplTypeSwizzler<T1, detail::DefaultGroup, ConfigData> final {
            using type = ConfigImpl<detail::DefaultGroup, T1>;
        };

        template <typename T1, typename T2, typename T3>
        struct ConfigImplTypeSwizzler<T1, T2, T3, typename std::enable_if<std::is_enum<T2>::value>::type> final {
            using type = ConfigImpl<T1, T2, T3>;
        };

        template <typename T1, typename T2, typename T3>
        struct ConfigImplTypeSwizzler<T1, T2, T3, typename std::enable_if<!std::is_enum<T2>::value>::type> final {
            using type = ConfigImpl<detail::DefaultGroup, T1, T2>;
        };
    }

    template <typename T1, typename T2 = detail::DefaultGroup, typename T3 = ConfigData>
    using Config = typename detail::ConfigImplTypeSwizzler<T1, T2, T3>::type;
}

#endif //CPP_CONFIG_CONFIG_HPP