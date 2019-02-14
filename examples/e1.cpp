#include <cppc/config.hpp>

enum class ConfigGroup {
    GENERAL, _SIZE
};

enum class ConfigID {
    WIDTH, HEIGHT, MODE, _SIZE
};

enum class WindowMode {
    FULLSCREEN, WINDOWED, WINDOWED_FULLSCREEN, _SIZE
};

struct ConfigData {
    std::variant<int, cppc::EnumeratedData<WindowMode>> value;

    std::string toString() const {
        switch (value.index()){
            case 0: return std::to_string(std::get<int>(value));
            case 1: return std::get<cppc::EnumeratedData<WindowMode>>(value).toString();
            default: assert(false); return "";
        }
    }

    static std::optional<ConfigData> parse(const std::string& s){
        auto opt = cppc::EnumeratedData<WindowMode>::parse(s);
        if (opt) return {{opt.value()}};
        else {
            char* end = nullptr;
            int i = std::strtol(s.c_str(), &end, 10);
            if (end == &s[0] + s.length()) return {{i}};
        }
        return std::nullopt;
    }

    template <typename T> T getValueField() const { return std::get<T>(value); }
    template <typename T> void setValueField(const T& t){ value = t; }
};

template <> WindowMode ConfigData::getValueField() const {
    return std::get<cppc::EnumeratedData<WindowMode>>(value);
}

void example1()
{
    cppc::configure<ConfigGroup >([](auto m){
        switch (m){
            case ConfigGroup::GENERAL: return "general";
            default: assert(false); return "";
        }
    });

    cppc::configure<ConfigID >([](auto m){
        switch (m){
            case ConfigID::WIDTH: return "width";
            case ConfigID::HEIGHT: return "height";
            case ConfigID::MODE: return "mode";
            default: assert(false); return "";
        }
    });

    cppc::configure<WindowMode>([](auto m){
        switch (m){
            case WindowMode::FULLSCREEN: return "fullscreen";
            case WindowMode::WINDOWED: return "windowed";
            case WindowMode::WINDOWED_FULLSCREEN: return "windowed_fullscreen";
            default: assert(false); return "";
        }
    });

    cppc::Config<ConfigGroup, ConfigID, ConfigData> c("examples/config1.ini");

    switch (c.get<WindowMode>(ConfigGroup::GENERAL, ConfigID::MODE).value()){
        case WindowMode::FULLSCREEN: std::cout << "fullscreen mode" << std::endl; break;
        case WindowMode::WINDOWED: std::cout << "windowed mode" << std::endl; break;
        case WindowMode::WINDOWED_FULLSCREEN: std::cout << "windowed fullscreen mode" << std::endl; break;
    }
}
