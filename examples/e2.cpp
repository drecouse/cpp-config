#include <cppc/config.hpp>

enum class ConfigGroup {
    GENERAL, RENDER, CAMERA, _SIZE
};

enum class ConfigID {
    WIDTH, HEIGHT, QUALITY, FULLSCREEN, SPEED, _SIZE
};

void example2()
{
    cppc::configure<ConfigGroup>([](auto e){
        switch (e){
            case ConfigGroup::GENERAL: return "general";
            case ConfigGroup::CAMERA: return "camera";
            case ConfigGroup::RENDER: return "render";
            default: assert(false); return "";
        }
    });

    cppc::configure<ConfigID>([](auto e){
        switch (e){
            case ConfigID::WIDTH: return "width";
            case ConfigID::HEIGHT: return "height";
            case ConfigID::QUALITY: return "quality";
            case ConfigID::FULLSCREEN: return "fullscreen";
            case ConfigID::SPEED: return "speed";
            default: assert(false); return "";
        }
    });

    cppc::Config<ConfigGroup, ConfigID> config("examples/config2.ini");

    std::cout << "Camera speed = " << config.get<double>(ConfigGroup::CAMERA, ConfigID::SPEED) << std::endl;
    std::cout << "Game speed = " << config.get<double>(ConfigGroup::GENERAL, ConfigID::SPEED) << std::endl;
    std::cout << "Quality is " << config.get<std::string>(ConfigGroup::RENDER, ConfigID::QUALITY) << std::endl;
    std::cout << "Fullscreen: " << config.get<bool>(ConfigGroup::RENDER, ConfigID::FULLSCREEN) << std::endl;

    auto width = config.bind<double>(ConfigGroup::CAMERA, ConfigID::WIDTH);

    std::cout << "Width is " << width << std::endl;
    width.update(width * 2);
    std::cout << "Width is now " << config.get<double>(ConfigGroup::CAMERA, ConfigID::WIDTH);

    config.clear(ConfigGroup::GENERAL, ConfigID::SPEED);
    config[ConfigGroup::GENERAL][ConfigID::SPEED] = 0;
    auto d = config[ConfigGroup::GENERAL][ConfigID::SPEED];

    config.save("config22.ini");
    config.clear();
}

