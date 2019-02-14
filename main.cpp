#include "config.hpp"

#include <cassert>

enum class MyConfig {
    Config1, Config2, _SIZE
};

enum class MyGroup {
    G1, _SIZE
};

struct Data {
    int a;

    std::string toString(){
        return std::to_string(a);
    }

    static Data parse(const std::string& s){
        return Data{atoi(s.c_str())};
    }

    template <typename T>
    T getValueField(){
        return a;
    }

    template <typename T>
    void setValueField(const T& t){
        a = t;
    }
};

int main()
{
    cppc::configure<MyConfig>([](MyConfig m){
        switch (m){
            case MyConfig::Config1: return "config1";
            case MyConfig::Config2: return "config2";
        }
        assert(false); return "";
    });

    cppc::configure<MyGroup>([](MyGroup m){
        switch (m){
            case MyGroup::G1: return "g1";
        }
        assert(false); return "";
    });

    cppc::Config<MyGroup, MyConfig, Data> c("c.ini");

    c.set(MyGroup::G1, MyConfig::Config1, 45356);
    c.save("c2.ini");

    return 0;
}
