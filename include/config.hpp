#ifndef CPP_CONFIG_CONFIG_HPP
#define CPP_CONFIG_CONFIG_HPP

#include <array>
#include <unordered_map>
#include <vector>
#include <optional>
#include <fstream>
#include <algorithm>
#include <functional>
#include <iostream>

namespace cppc
{
    namespace detail
    {
        template <typename NAME, NAME SIZE = NAME::_SIZE>
        struct ConfigDataHolder final {
            static std::unordered_map<std::string, NAME> nameToType;
            static std::array<std::string, static_cast<size_t>(SIZE)> typeToName;
        };

        template<typename NAME, NAME SIZE>
        std::unordered_map<std::string, NAME> ConfigDataHolder<NAME, SIZE>::nameToType;
        template<typename NAME, NAME SIZE>
        std::array<std::string, static_cast<size_t>(SIZE)> ConfigDataHolder<NAME, SIZE>::typeToName;

        template <typename CONFIG, typename T>
        class ConfigBoundVariable final {
        public:
            ConfigBoundVariable(CONFIG& config, typename CONFIG::GroupName groupName, typename CONFIG::ConfigName configName)
                : config{config}
                , groupName{groupName}
                , configName{configName}
            {}

            operator T() const {
                return config.template get<T>(groupName, configName).value();
            }

            void update(const T& t) {
                config.template set<T>(groupName, configName, t);
            }
        private:
            CONFIG& config;
            typename CONFIG::GroupName groupName;
            typename CONFIG::ConfigName configName;
        };

        inline void trim(std::string &s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
                return !std::isspace(ch);
            }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
                return !std::isspace(ch);
            }).base(), s.end());
        }
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
            detail::ConfigDataHolder<ENUM, SIZE>::nameToType[name] = type;
            detail::ConfigDataHolder<ENUM, SIZE>::typeToName[i] = name;
        }
    }

    // Helper struct to use typed enums for configuration values as well
    template <typename ENUM, ENUM SIZE = ENUM::_SIZE>
    struct EnumeratedData final {
        ENUM value;

        EnumeratedData(ENUM value) : value{value} {}
        operator ENUM() const { return value; }

        std::string toString() const {
            return detail::ConfigDataHolder<ENUM, SIZE>::typeToName[static_cast<size_t>(value)];
        }

        static std::optional<EnumeratedData> parse(const std::string& s) {
            auto& map = detail::ConfigDataHolder<ENUM, SIZE>::nameToType;
            auto found = map.find(s);
            if (found == map.end()) return std::nullopt;
            else return std::make_optional(EnumeratedData{found->second});
        }

        template <typename T, typename = void> T getValueField() const;
        template <typename = void> ENUM getValueField() const { return value; }
        template <typename T, typename = void> void setValueField(const T& t);
        template <typename = void> void setValueField(const ENUM& t){ value = t; }
    };

    // Represents a collection of grouped key-value pairs.
    // The template parameters defines the enums used to access the values, and stored data type.
    template <typename GROUP, typename NAME, typename DATA, GROUP GROUP_SIZE = GROUP::_SIZE, NAME NAME_SIZE = NAME::_SIZE>
    class Config final {
    public:
        using GroupName = GROUP;
        using ConfigName = NAME;
        using DataType = DATA;

    public:
        explicit Config(const std::string& filePath) {
            load(filePath);
        }

        Config& load(const std::string& filePath){
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
                                configValues[actGroup][static_cast<size_t>(n.value())] = opt;
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

        Config& load(const Config& config) {
            for (size_t i = 0; i < static_cast<size_t>(GROUP_SIZE); ++i){
                for (size_t j = 0; j < static_cast<size_t>(NAME_SIZE); ++j){
                    if (config.configValues[i][j]){
                        configValues[i][j] = config.configValues[i][j];
                    }
                }
            }
            return *this;
        }

        bool save(const std::string& filePath) {
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

        void clear() {
            for (auto& group : configValues) {
                for (auto& config : group){
                    config = std::nullopt;
                }
            }
        }

        template <typename T>
        detail::ConfigBoundVariable<Config, T> bind(GroupName groupName, ConfigName configName) {
            return detail::ConfigBoundVariable<Config, T>{*this, groupName, configName};
        }

        template <typename T>
        std::optional<T> get(GroupName groupName, ConfigName configName) {
            auto& opt = configValues[static_cast<size_t>(groupName)][static_cast<size_t>(configName)];
            return opt ? std::make_optional(opt.value().template getValueField<T>()) : std::nullopt;
        }

        template <typename T>
        void set(GroupName groupName, ConfigName configName, const T& t) {
            auto& opt = configValues[static_cast<size_t>(groupName)][static_cast<size_t>(configName)];
            if (!opt){
                configValues[static_cast<size_t>(groupName)][static_cast<size_t>(configName)] = std::make_optional(DATA{t});
            } else {
                opt.value().template setValueField(t);
            }
        }

        std::optional<std::reference_wrapper<DATA>> getData(GroupName groupName, ConfigName configName) {
            auto& opt = configValues[static_cast<size_t>(groupName)][static_cast<size_t>(configName)];
            return opt ? std::make_optional(std::reference_wrapper<DATA>{opt.value()}) : std::nullopt;
        }

    private:
        std::string toString(ConfigName name){
            return detail::ConfigDataHolder<ConfigName, NAME_SIZE>::typeToName[static_cast<size_t>(name)];
        }

        std::string toString(GroupName group){
            return detail::ConfigDataHolder<GroupName, GROUP_SIZE>::typeToName[static_cast<size_t>(group)];
        }

        std::optional<ConfigName> configNameFromString(const std::string& str){
            auto& map = detail::ConfigDataHolder<NAME, NAME_SIZE>::nameToType;
            auto found = map.find(str);
            if (found == map.end()) return std::nullopt;
            return std::make_optional(found->second);
        }

        std::optional<GroupName> configGroupFromString(const std::string& str){
            auto& map = detail::ConfigDataHolder<GROUP, GROUP_SIZE>::nameToType;
            auto found = map.find(str);
            if (found == map.end()) return std::nullopt;
            return std::make_optional(found->second);
        }

    private:
        std::array<std::array<std::optional<DATA>, static_cast<size_t>(NAME_SIZE)>, static_cast<size_t>(GROUP_SIZE)> configValues;
    };
}

// TODO
/*
 * test
 * example
 */

#endif //CPP_CONFIG_CONFIG_HPP
