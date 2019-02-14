#include "config.hpp"

#include <cassert>
#include <variant>

enum class MyConfig {
    Config1, Config2, _SIZE
};

enum class MyGroup {
    G1, _SIZE
};

enum class MyValueType {
    V1, V2, _SIZE
};

struct Data {
    std::variant<int, cppc::EnumeratedData<MyValueType>> a;

    std::string toString() const {
        switch (a.index()){
            case 0: return std::to_string(std::get<int>(a));
            case 1: return std::get<cppc::EnumeratedData<MyValueType>>(a).toString();
        }
        assert(false); return "";
    }

    static std::optional<Data> parse(const std::string& s){
        auto opt = cppc::EnumeratedData<MyValueType>::parse(s);
        if (opt) return {{opt.value()}};
        else return {{std::atoi(s.c_str())}};
    }

    template <typename T>
    T getValueField() const {
        return std::get<T>(a);
    }

    template <typename T>
    void setValueField(const T& t){
        a = t;
    }
};

template <> MyValueType Data::getValueField() const {
    return std::get<cppc::EnumeratedData<MyValueType>>(a);
}

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

    cppc::configure<MyValueType >([](MyValueType m){
        switch (m){
            case MyValueType::V1: return "v1";
            case MyValueType::V2: return "v2";
        }
        assert(false); return "";
    });

    cppc::Config<MyGroup, MyConfig, Data> c("c.ini");

    c.set(MyGroup::G1, MyConfig::Config1, 45356);
    c.save("c2.ini");

    auto b = c.bind<MyValueType>(MyGroup::G1, MyConfig::Config2);
    MyValueType m = b;
    std::cout << (int)m;
    b.update(MyValueType::V2);
    c.save("c2.ini");
    c.clear();
    return 0;
}
