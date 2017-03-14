#include <iostream>
#include <fstream>
#include <vector>
#include <typeinfo>
#include <cstring>
#include "eipp.h"

class ContentLoader {
public:
    ContentLoader(std::string file_name) {
        std::ifstream f(file_name, std::ios::in|std::ios::binary|std::ios::ate);
        auto size = f.tellg();
        f.seekg(0, std::ios::beg);

        buf_ = new char[size];
        f.read(buf_, size);
        f.close();
    }

    ~ContentLoader() {
        if(buf_) {
            delete[] buf_;
        }
    }

    char* get_buf() {
        return buf_;
    }

private:
    char* buf_ = nullptr;
};

int test_case1() {
    std::cout << std::endl << "test case 1" << std::endl;

    ContentLoader cl("./test_data/case1");
    auto content = cl.get_buf();

    eipp::EIDecoder decoder(content);
    auto result = decoder.parse<eipp::Long>();
    if(!decoder.is_valid()) {
        return -1;
    }

    std::cout << typeid(result).name() << ", " << result << std::endl;
    if( result != 101) {
        return -2;
    }

    return 0;
}

int test_case2() {
    std::cout << std::endl << "test case 2" << std::endl;

    ContentLoader cl("./test_data/case2");
    auto content = cl.get_buf();

    using T = eipp::Tuple<eipp::String, eipp::Binary, eipp::Long, eipp::Double >;

    eipp::EIDecoder decoder(content);
    auto result = decoder.parse<T>();
    if(!decoder.is_valid()) {
        return -1;
    }

    auto v1 = result->get<0>();
    auto v2 = result->get<1>();
    auto v3 = result->get<2>();
    auto v4 = result->get<3>();

    std::cout << typeid(v1).name() << ", " << v1 << std::endl;
    std::cout << typeid(v2).name() << ", " << v2 << std::endl;
    std::cout << typeid(v3).name() << ", " << v3 << std::endl;
    std::cout << typeid(v4).name() << ", " << v4 << std::endl;

    if(strncmp(v1, "v1string", 8)) {
        return -2;
    }

    if(strncmp(v2, "v2binary", 8)) {
        return -2;
    }

    if(v3 != 222) {
        return -2;
    }

    if(v4 - 1.23 > 0.01) {
        return -2;
    }

    return 0;
}


int test_case3() {
    std::cout << std::endl << "test case 3" << std::endl;

    ContentLoader cl("./test_data/case3");
    auto content = cl.get_buf();

    using T = eipp::Tuple<eipp::String, eipp::Long>;
    using T1 = eipp::List<T>;

    eipp::EIDecoder decoder(content);
    auto result = decoder.parse<T1>();
    if(!decoder.is_valid()) {
        return -1;
    }


    for(auto tuple: *result) {
        auto v1 = tuple->get<0>();
        auto v2 = tuple->get<1>();

        std::cout << typeid(v1).name() << ", " << v1 << std::endl;
        std::cout << typeid(v2).name() << ", " << v2 << std::endl;
    }

    return 0;
}

int test_case4() {
    std::cout << std::endl << "test case 4" << std::endl;

    ContentLoader cl("./test_data/case4");
    auto content = cl.get_buf();

    using T = eipp::Tuple<eipp::Long, eipp::Long, eipp::String>;
    using T1 = eipp::Map<eipp::Long, T>;

    eipp::EIDecoder decoder(content);
    auto result = decoder.parse<T1>();
    if(!decoder.is_valid()) {
        return -1;
    }

    for(auto& iter: result->value) {
        std::cout << typeid(iter.first).name() << ", " << iter.first << std::endl;
        auto tuple = iter.second;
        auto v1 = tuple->get<0>();
        auto v2 = tuple->get<1>();
        auto v3 = tuple->get<2>();

        std::cout << "    " << typeid(v1).name() << ", " << v1 << std::endl;
        std::cout << "    " << typeid(v2).name() << ", " << v2 << std::endl;
        std::cout << "    " << typeid(v3).name() << ", " << v3 << std::endl;
    }

    return 0;
}





typedef int(*test_func_t)();

int main() {
    int ret;
    std::vector<test_func_t> funcs{
            test_case1, test_case2, test_case3, test_case4,
    };

    for(test_func_t func: funcs) {
        ret = func();
        if(ret == -1) {
            std::cerr << "parse error!" << std::endl;
            return -1;
        } else if(ret == -2) {
            std::cerr << "compare failure!" << std::endl;
            return -2;
        }
    }

    return 0;

}