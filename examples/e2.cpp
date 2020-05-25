#include <cppc/config.hpp>
#include <iostream>

enum class ConfigGroup {
    GENERAL, RENDER, CAMERA, _SIZE
};

enum class GeneralID { SPEED, _SIZE };
enum class RenderID { QUALITY, FULLSCREEN, _SIZE };
enum class CameraID { WIDTH, HEIGHT, SPEED, _SIZE };

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

    cppc::configure<GeneralID>([](auto e){
        switch (e){
            case GeneralID::SPEED: return "speed";
            default: assert(false); return "";
        }
    });

    cppc::configure<RenderID>([](auto e){
        switch (e){
            case RenderID::QUALITY: return "quality";
            case RenderID::FULLSCREEN: return "fullscreen";
            default: assert(false); return "";
        }
    });

    cppc::configure<CameraID>([](auto e){
        switch (e){
            case CameraID::WIDTH: return "width";
            case CameraID::HEIGHT: return "height";
            case CameraID::SPEED: return "speed";
            default: assert(false); return "";
        }
    });

    cppc::detail::ConfigImpl<
            ConfigGroup,
            std::pair<GeneralID, cppc::DefaultConfigData>,
            std::pair<RenderID, cppc::DefaultConfigData>,
            std::pair<CameraID, cppc::DefaultConfigData>
    > config("examples/config2.ini");

   // config[std::pair{ConfigGroup::GENERAL, GeneralID::SPEED}] = 11.2;

    config.set(ConfigGroup::GENERAL, GeneralID::SPEED, 11.2);

    std::string str = config[ConfigGroup::RENDER][RenderID::QUALITY];

    int speed2 = config[ConfigGroup::CAMERA][CameraID::SPEED];

   // config.clear(ConfigGroup::RENDER, RenderID::QUALITY);

 //   auto speed = config.bind<double>(ConfigGroup::CAMERA, CameraID::SPEED);
 //   std::cout << speed << std::endl;
 //   speed.update(30);

    std::cout << config.get<bool>(ConfigGroup::RENDER, RenderID::FULLSCREEN) << std::endl;



    config.save("config22.ini");
    return;

/*
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
    */
}







