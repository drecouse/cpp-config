#ifndef CPP_CONFIG_CONFIG_HPP
#define CPP_CONFIG_CONFIG_HPP

#include <array>
#include <unordered_map>
#include <vector>
#include <optional>
#include <fstream>
#include <algorithm>
#include <functional>

namespace cppc
{
    namespace detail
    {
        template <typename NAME, NAME SIZE = NAME::_SIZE>
        struct ConfigDataHolder {
            static std::unordered_map<std::string, NAME> nameToType;
            static std::array<std::string, static_cast<size_t>(SIZE)> typeToName;
        };

        template<typename NAME, NAME SIZE>
        std::unordered_map<std::string, NAME> ConfigDataHolder<NAME, SIZE>::nameToType;
        template<typename NAME, NAME SIZE>
        std::array<std::string, static_cast<size_t>(SIZE)> ConfigDataHolder<NAME, SIZE>::typeToName;

        template <typename CONFIG, typename T>
        class ConfigBoundVariable {
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

    template <typename NAME, NAME SIZE = NAME::_SIZE, typename F>
    void configure(const F& getName){
        for (size_t i = 0; i < static_cast<size_t>(SIZE); ++i){
            auto type = static_cast<NAME>(i);
            auto name = getName(type);
            detail::ConfigDataHolder<NAME, SIZE>::nameToType[name] = type;
            detail::ConfigDataHolder<NAME, SIZE>::typeToName[i] = name;
        }
    }

    template <typename GROUP, typename NAME, typename DATA, GROUP GROUP_SIZE = GROUP::_SIZE, NAME NAME_SIZE = NAME::_SIZE>
    class Config {
    public:
        using GroupName = GROUP;
        using ConfigName = NAME;

    public:
        explicit Config(const std::string& filePath) {
            load(filePath);
        }

        Config& load(const std::string& filePath){
            std::fstream f(filePath);

            size_t actGroup = 0;
            std::string line;
            while (std::getline(f, line)){
                detail::trim(line);
                if (line.length() == 0 || line[0] == ';'){
                    continue;
                }
                else if (line[0] == '['){
                    auto found = std::find(line.begin(), line.end(), ']');
                    if (found != line.end()){
                        auto s = line.substr(1, found - line.begin() - 1);
                        detail::trim(s);
                        auto g = configGroupFromString(s);
                        if (g){
                            actGroup = static_cast<size_t>(g.value());
                        } else {
                            // TODO
                        }
                    } else {
                        //TODO
                    }
                } else {
                    auto found = std::find(line.begin(), line.end(), '=');
                    if (found != line.end()){
                        auto s = line.substr(0, found - line.begin());
                        detail::trim(s);
                        auto n = configNameFromString(s);
                        if (n){
                            s = std::string{found + 1, line.end()};
                            detail::trim(s);
                            configValues[actGroup][static_cast<size_t>(n.value())] = DATA::parse(s);
                        } else {
                            //TODO
                        }
                    } else {
                        //TODO
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

        bool save(const std::string& filePath){
            std::ofstream f(filePath);
            if (!f) return false;

            for (size_t i = 0; i < static_cast<size_t>(GROUP_SIZE); ++i){
                f << '[' << toString(static_cast<GroupName >(i)) << ']' << std::endl;
                for (size_t j = 0; j < static_cast<size_t>(NAME_SIZE); ++j){
                    if (configValues[i][j]){
                        f << toString(static_cast<ConfigName >(j)) << "=" << configValues[i][j].value().toString() << std::endl;
                    }
                }
                f << std::endl;
            }
            return true;
        }

        template <typename T>
        detail::ConfigBoundVariable<Config, T> bind(GroupName groupName, ConfigName configName) {
            return detail::ConfigBoundVariable<Config, T>{*this, groupName, configName};
        }

        template <typename T>
        std::optional<T> get(GroupName groupName, ConfigName configName) {
            auto& opt = getData(groupName, configName);
            return opt ? std::make_optional(opt.value().get().template getValueField<T>()) : std::nullopt;
        }

        template <typename T>
        void set(GroupName groupName, ConfigName configName, const T& t) {
            const auto& opt = getData(groupName, configName);
            if (!opt){
                configValues[static_cast<size_t>(groupName)][static_cast<size_t>(configName)] = std::make_optional(DATA{t});
            } else {
                opt.value().get().template setValueField(t);
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
 * error messages
 * comment
 * optional get?
 * test
 */

#endif //CPP_CONFIG_CONFIG_HPP
