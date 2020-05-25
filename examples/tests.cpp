#include "cppc/config.hpp"
#include <iostream>

enum class WindowMode {
    FULLSCREEN, WINDOWED, WINDOWED_FULLSCREEN, _SIZE
};

using CustomComplexData = cppc::ComplexData<bool, std::string, cppc::EnumeratedData<WindowMode>>;

struct CustomConfigData {
    std::variant<int, cppc::EnumeratedData<WindowMode>, CustomComplexData> value;
    using AvailableValueTypes = std::tuple<int, cppc::EnumeratedData<WindowMode>, CustomComplexData>;

    std::string toString() const {
        switch (value.index()){
            case 0: return std::to_string(std::get<int>(value));
            case 1: return std::get<cppc::EnumeratedData<WindowMode>>(value).toString();
            case 2: return std::get<CustomComplexData>(value).toString();
            default: assert(false); return "";
        }
    }

    static std::optional<CustomConfigData> parse(const std::string& s){
        auto opt = cppc::EnumeratedData<WindowMode>::parse(s);
        if (opt) return {{opt.value()}};

        auto oc = CustomComplexData::parse(s);
        if (oc) return std::optional<CustomConfigData>{CustomConfigData{oc.value()}};

        char* end = nullptr;
        int i = std::strtol(s.c_str(), &end, 10);
        if (end == &s[0] + s.length()) return {{i}};

        return std::nullopt;
    }

    template <typename T> const T& getValueField() const noexcept { assert(std::holds_alternative<T>(value)); return std::get<T>(value); }
    template <typename T> void setValueField(T&& t) { value = std::forward<T>(t); }
};

enum class ConfigGroup {
    GENERAL, RENDER, CAMERA, _SIZE
};

enum class GeneralID { SPEED, MASS, _SIZE };
enum class RenderID { MODE, TEST, _SIZE };
enum class CameraID { WIDTH, HEIGHT, SPEED, NAME, FOV, _SIZE };

void init_tests() {
    cppc::configure<ConfigGroup>([](auto e) {
        switch (e) {
            case ConfigGroup::GENERAL: return "general";
            case ConfigGroup::CAMERA: return "camera";
            case ConfigGroup::RENDER: return "render";
            default: assert(false); return "";
        }
    });

    cppc::configure<GeneralID>([](auto e) {
        switch (e) {
            case GeneralID::SPEED: return "speed";
            case GeneralID::MASS: return "mass";
            default: assert(false); return "";
        }
    });

    cppc::configure<RenderID>([](auto e) {
        switch (e) {
            case RenderID::MODE: return "mode";
            case RenderID::TEST: return "test";
            default: assert(false); return "";
        }
    });

    cppc::configure<CameraID>([](auto e) {
        switch (e) {
            case CameraID::WIDTH: return "width";
            case CameraID::HEIGHT: return "height";
            case CameraID::SPEED: return "speed";
            case CameraID::NAME: return "name";
            case CameraID::FOV: return "fov";
            default: assert(false); return "";
        }
    });

    cppc::configure<WindowMode>([](auto e){
        switch (e){
            case WindowMode::FULLSCREEN: return "fullscreen";
            case WindowMode::WINDOWED: return "windowed";
            case WindowMode::WINDOWED_FULLSCREEN: return "windowed_fullscreen";
            default: assert(false); return "";
        }
    });
}

void create_test() {
    cppc::Config<GeneralID> c1;
    cppc::Config<std::pair<GeneralID, CustomConfigData>> c2;
    cppc::Config<ConfigGroup, GeneralID, RenderID> c3;
    cppc::Config<
            ConfigGroup,
            std::pair<GeneralID, CustomConfigData>,
            CameraID,
            std::pair<RenderID, cppc::DefaultConfigData>
    > c4;
}

void run_test() {
    cppc::Config<
            ConfigGroup,
            std::pair<GeneralID, cppc::DefaultConfigData>,
            std::pair<RenderID, CustomConfigData>,
            CameraID
    > c2;
    c2.setErrorFunction([](const auto& s){ std::cerr << s << std::endl; });
    auto c22 = c2;
    c2.load("examples/config2.ini");

    c2.setIfEmpty(ConfigGroup::GENERAL, GeneralID::SPEED, 10);
    c2.setIfEmpty(ConfigGroup::GENERAL, GeneralID::MASS, 1020);
    c2.setIfEmpty(ConfigGroup::CAMERA, CameraID::SPEED, 10);
    c2.setIfEmpty(ConfigGroup::CAMERA, CameraID::FOV, 90);
    c2.setIfEmpty(ConfigGroup::CAMERA, CameraID::HEIGHT, 1920);
    c2.setIfEmpty(ConfigGroup::CAMERA, CameraID::NAME, "defaultcam");
    c2.setIfEmpty(ConfigGroup::CAMERA, CameraID::WIDTH, 1080);
    c2.setIfEmpty(ConfigGroup::RENDER, RenderID::MODE, WindowMode::FULLSCREEN);
    c2.setIfEmpty(ConfigGroup::RENDER, RenderID::TEST, cppc::make_complex_data(true, std::string{"0101"}, WindowMode::WINDOWED));

    assert(c2.filled());
    assert(!c22.filled());

    auto c4 = c2;
    assert(c4.filled());
    c2 = c4;
    assert(c2.filled());

    assert(c2[ConfigGroup::RENDER][RenderID::MODE] == WindowMode::WINDOWED_FULLSCREEN);
    assert(c2[ConfigGroup::CAMERA][CameraID::NAME] == std::string{"cam_first"});

    c2[GeneralID::MASS] = 14;

    c2.commit(ConfigGroup::RENDER, RenderID::TEST)
      .commit(ConfigGroup::GENERAL, GeneralID::MASS)
      .forget(ConfigGroup::CAMERA, CameraID::NAME)
      .save("config22.ini");

    c4 = c22;
    c4.load("config22.ini", true);
    c4.setIfEmpty(ConfigGroup::CAMERA, CameraID::NAME, "defaultcam");
    c4.load(c2, false);
    assert(c4.filled());

    assert(c2[ConfigGroup::CAMERA][CameraID::NAME] != c4[ConfigGroup::CAMERA][CameraID::NAME].as<std::string>());
    assert(c2[ConfigGroup::GENERAL][GeneralID::MASS] == (int)c4[ConfigGroup::GENERAL][GeneralID::MASS]);
}