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


int test_case5() {
    std::cout << std::endl << "test case 5" << std::endl;

    ContentLoader cl("./test_data/case5");
    auto content = cl.get_buf();

    using T = eipp::Tuple<eipp::Long, eipp::Long, eipp::String>;
    using T1 = eipp::Map<eipp::Long, T>;
    using T2 = eipp::MultiTypeList<eipp::Long, T, T1>;

    eipp::EIDecoder decoder(content);
    auto result = decoder.parse<T2>();
    if(!decoder.is_valid()) {
        return -1;
    }


    auto v1 = result->get<0>();
    auto v2 = result->get<1>();
    auto v3 = result->get<2>();

    std::cout << v1 << std::endl;

    if(v1 != 555) {
        return -2;
    }

    auto tv1 = v2->get<0>();
    auto tv2 = v2->get<1>();
    auto tv3 = v2->get<2>();

    std::cout << tv1 << std::endl;
    std::cout << tv2 << std::endl;
    std::cout << tv3 << std::endl;

    if (tv1 != 1) {
        return -2;
    }

    if (tv2 != 2) {
        return -2;
    }

    if (tv3 != "case5") {
        return -2;
    }


    for(auto& iter: *v3) {
        std::cout << typeid(iter.first).name() << ", " << iter.first << std::endl;
        auto tuple = iter.second;
        auto mv1 = tuple->get<0>();
        auto mv2 = tuple->get<1>();
        auto mv3 = tuple->get<2>();

        std::cout << "    " << typeid(mv1).name() << ", " << mv1 << std::endl;
        std::cout << "    " << typeid(mv2).name() << ", " << mv2 << std::endl;
        std::cout << "    " << typeid(mv3).name() << ", " << mv3 << std::endl;
    }

    return 0;
}





typedef int(*test_func_t)();

int main() {
//    int ret;
//    std::vector<test_func_t> funcs{
//            test_case1, test_case2, test_case3, test_case4,
//            test_case5,
//    };
//
//    for(test_func_t func: funcs) {
//        ret = func();
//        if(ret == -1) {
//            std::cerr << "parse error!" << std::endl;
//            return -1;
//        } else if(ret == -2) {
//            std::cerr << "compare failure!" << std::endl;
//            return -2;
//        }
//    }

//    int number;
//    std::cin >> number;
//    std::cout << "number is: " << number << std::endl;


    char *s1 = "Hello";
    std::string s2("World!");

    eipp::EIEncoder en;
//    en.start_encode_list();
//    en.encode(10);
//    en.encode(29LL);
//    en.encode(s1, eipp::TYPE::Binary);
//    en.encode(s2, eipp::TYPE::Binary);
//    en.end_encode_list();

//    std::list<int> ll;
//    ll.push_back(101);
//    ll.push_back(202);
//    ll.push_back(303);
//    auto t = std::make_tuple(10, 29LL, s1, s2, ll);
//    en.encode(t);

//    std::list<std::tuple<int, int, std::string>> lt;
//    lt.push_back(std::make_tuple(10, 101, "One"));
//    lt.push_back(std::make_tuple(20, 202, "Two"));
//    lt.push_back(std::make_tuple(30, 303, "Three"));
//
//    en.encode(lt);

    typedef std::tuple<int, std::string> id_t;
    std::tuple<int, std::string, std::list<id_t>, std::map<int, id_t>> data;

    std::get<0>(data) = 101;
    std::get<1>(data) = "Nihao";

    auto& l3 = std::get<2>(data);
    l3.push_back(std::make_tuple(1, "One"));
    l3.push_back(std::make_tuple(2, "Two"));
    l3.push_back(std::make_tuple(3, "Three"));

    auto& m4 = std::get<3>(data);
    m4.insert(std::make_pair(1000, std::make_tuple(1000, "AAAA")));
    m4.insert(std::make_pair(2000, std::make_tuple(2000, "BBBB")));
    m4.insert(std::make_pair(3000, std::make_tuple(3000, "CCCC")));

    en.encode(data);

    std::cout << "encode done" << std::endl;

    auto s = en.get_data();
    std::ofstream f("/tmp/eippout");
    f << s;
    f.close();

    return 0;

}