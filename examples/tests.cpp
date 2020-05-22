#include "cppc/config.hpp"
#include <iostream>

enum class WindowMode {
    FULLSCREEN, WINDOWED, WINDOWED_FULLSCREEN, _SIZE
};

struct CustomConfigData {
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

    static std::optional<CustomConfigData> parse(const std::string& s){
        auto opt = cppc::EnumeratedData<WindowMode>::parse(s);
        if (opt) return {{opt.value()}};

        auto oc = cppc::ComplexData<bool, std::string, cppc::EnumeratedData<WindowMode>>::parse(s);
        if (oc) return std::optional<CustomConfigData>{CustomConfigData{oc.value()}};

        char* end = nullptr;
        int i = std::strtol(s.c_str(), &end, 10);
        if (end == &s[0] + s.length()) return {{i}};

        return std::nullopt;
    }

    template <typename T> T getValueField() const { return std::get<T>(value); }
    template <typename T> void setValueField(const T& t){ value = t; }
};

enum class ConfigGroup {
    GENERAL, RENDER, CAMERA, _SIZE
};

enum class GeneralID { SPEED, MASS, _SIZE };
enum class RenderID { MODE, TEST, _SIZE };
enum class CameraID { WIDTH, HEIGHT, SPEED, NAME, FOV, _SIZE };

void init_tests() {
    cppc::configure<ConfigGroup>([](auto e){
        switch (e){
            case ConfigGroup::GENERAL: return "general";
            case ConfigGroup::CAMERA: return "camera";
            case ConfigGroup::RENDER: return "render";
            default: assert(false); return "";
        }
    });

    cppc::configure<GeneralID>([](auto e){
        switch (e){
            case GeneralID::SPEED: return "speed";
            case GeneralID::MASS: return "mass";
            default: assert(false); return "";
        }
    });

    cppc::configure<RenderID>([](auto e){
        switch (e){
            case RenderID::MODE: return "mode";
            case RenderID::TEST: return "test";
            default: assert(false); return "";
        }
    });

    cppc::configure<CameraID>([](auto e){
        switch (e){
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
    c22[ConfigGroup::CAMERA][CameraID::HEIGHT] = 600;
    c22[ConfigGroup::CAMERA][CameraID::WIDTH] = 1;
    c2.load(c22, false);

    if (c2[ConfigGroup::CAMERA][CameraID::HEIGHT] == 600 &&
        c2[ConfigGroup::CAMERA][CameraID::WIDTH] == 800){
        std::cout << "Load test passed" << std::endl;
    } else {
        std::cout << "Load test failed" << std::endl;
    }

    c2[GeneralID::SPEED] = 13.5;

    auto boundSpeed = c2.bind<double>(ConfigGroup::GENERAL, GeneralID::SPEED);
    std::cout << boundSpeed << std::endl;
    boundSpeed.update(32.0);

    c2.save("config22.ini");

    auto c4 = std::move(c2);
    c2 = c4;
    c2 = std::move(c4);
}