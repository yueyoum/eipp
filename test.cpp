#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <tuple>
#include <map>
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

    if(v1 != "v1string") {
        return -2;
    }

    if(v2 != "v2binary") {
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

    using T = eipp::Tuple<eipp::Atom, eipp::Long>;
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

    for(auto& iter: *result) {
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
    // decode test
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


    // encode test
    eipp::EIEncoder en;

    typedef std::tuple<int, std::string, std::list<eipp::Atom>> Person_t;
    typedef std::map<eipp::Binary, std::list<int>> T1;

    std::tuple<std::string, int, Person_t, std::vector<Person_t>, T1> data;

    std::get<0>(data) = "Hello World!";
    std::get<1>(data) = 101;

    auto p = std::make_tuple(1, "Jim", std::list<eipp::Atom>{eipp::Atom("j1"), eipp::Atom("j2")});
    std::get<2>(data) = p;

    Person_t lp1;
    std::get<0>(lp1) = 2;
    std::get<1>(lp1) = "Tom";
    std::get<2>(lp1).push_back(eipp::Atom("t1"));
    std::get<2>(lp1).push_back(eipp::Atom("t2"));
    std::get<2>(lp1).push_back(eipp::Atom("t3"));

    std::get<3>(data).push_back(lp1);


    Person_t lp2;
    std::get<0>(lp2) = 3;
    std::get<1>(lp2) = "David";
    std::get<2>(lp2).push_back(eipp::Atom("d1"));
    std::get<2>(lp2).push_back(eipp::Atom("d2"));
    std::get<2>(lp2).push_back(eipp::Atom("d3"));

    std::get<3>(data).push_back(lp2);

    std::get<4>(data).insert(std::make_pair(eipp::Binary("binary 1"), std::list<int>{1,2,3}));
    std::get<4>(data).insert(std::make_pair(eipp::Binary("binary 2"), std::list<int>{4,5,6}));
    std::get<4>(data).insert(std::make_pair(eipp::Binary("binary 3"), std::list<int>{7,8,9}));

    en.encode(data);

    std::cout << "encode done" << std::endl;

    auto s = en.get_data();
    std::ofstream f("/tmp/eippout");
    f << s;
    f.close();

    return 0;

}