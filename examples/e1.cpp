#include <cppc/config.hpp>

enum class ConfigGroup {
    GENERAL, VIDEO, _SIZE
};

enum class ConfigID {
    WIDTH, HEIGHT, MODE, TEST, _SIZE
};

enum class WindowMode {
    FULLSCREEN, WINDOWED, WINDOWED_FULLSCREEN, _SIZE
};

struct ConfigData {
    std::variant<int, cppc::EnumeratedData<WindowMode>, cppc::ComplexData<bool, std::string, cppc::EnumeratedData<WindowMode>>> value;
    using AvailableValueTypes = std::tuple<int, cppc::EnumeratedData<WindowMode>, cppc::ComplexData<bool, std::string, cppc::EnumeratedData<WindowMode>>>;

    std::string toString() const {
        switch (value.index()){
            case 0: return std::to_string(std::get<int>(value));
            case 1: return std::get<cppc::EnumeratedData<WindowMode>>(value).toString();
            case 2: return std::get<cppc::ComplexData<bool, std::string, cppc::EnumeratedData<WindowMode>>>(value).toString();
            default: assert(false); return "";
        }
    }

    static std::optional<ConfigData> parse(const std::string& s){
        auto opt = cppc::EnumeratedData<WindowMode>::parse(s);
        if (opt) return {{opt.value()}};

        auto oc = cppc::ComplexData<bool, std::string, cppc::EnumeratedData<WindowMode>>::parse(s);
        if (oc) return std::optional<ConfigData>{ConfigData{oc.value()}};

        char* end = nullptr;
        int i = std::strtol(s.c_str(), &end, 10);
        if (end == &s[0] + s.length()) return {{i}};

        return std::nullopt;
    }

    template <typename T> T getValueField() const { return std::get<T>(value); }
    template <typename T> void setValueField(const T& t){ value = t; }
};

struct Cmp {
    bool b ; std::string s; cppc::EnumeratedData<WindowMode> e;
};

template <> WindowMode ConfigData::getValueField() const {
    return std::get<cppc::EnumeratedData<WindowMode>>(value);
}

void example1()
{
    /*
    cppc::configure<ConfigGroup>([](auto m){
        switch (m){
            case ConfigGroup::GENERAL: return "general";
            default: assert(false); return "";
        }
    });

    cppc::configure<ConfigID>([](auto m){
        switch (m){
            case ConfigID::WIDTH: return "width";
            case ConfigID::HEIGHT: return "height";
            case ConfigID::MODE: return "mode";
            case ConfigID::TEST: return "test";
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


    cppc::ConfigImpl<ConfigGroup, ConfigID, cppc::detail::AutoConverter<cppc::ConfigImpl<ConfigID, ConfigData, ConfigID::_SIZE>>>
    ConfigImpl<Group, std::variant<ConfigImpl<Id1, Data1>, ConfigImpl<Id2, Data2>>>

    cppc::EnumMap<ConfigID, ConfigData> c("examples/config1.ini");

    c[ConfigID::MODE] = WindowMode::WINDOWED;

    WindowMode ww = c[ConfigID::MODE];

 //   Cmp cccc = cppc::ComplexData<bool, std::string, cppc::EnumeratedData<WindowMode>>{};
    Cmp ccscc = c[ConfigID::TEST];

    switch (c.get<WindowMode>(ConfigID::MODE)){
        case WindowMode::FULLSCREEN: std::cout << "fullscreen mode" << std::endl; break;
        case WindowMode::WINDOWED: std::cout << "windowed mode" << std::endl; break;
        case WindowMode::WINDOWED_FULLSCREEN: std::cout << "windowed fullscreen mode" << std::endl; break;
        default: break;
    }

    c.save("config11.ini");
*/
}
