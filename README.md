# EIPP

A C++11 Header Only Library for using erlang [ei][1] library.

It's a wrapper of `ei.h`.

The Goal of EIPP is interactive between Erlang system and other Language.

e.g. Write c++ driver for erlang, or encode/decode [External Term Format][2]
                                                   
The library **ONLY** support the following types:

*   long: (include char, short, int, long)
    *   `eipp::Long`
*   double. (include float, double)
    *   `eipp::Double`
*   string. (byte sequence, byte in 0-255)
    *   `eipp::String`
*   binary. (byte sequence, any bytes)
    *   `eipp::Binary`
*   atom. (same as string for cpp)
    *   `eipp::Atom`
*   list. (same type elements, any type supported here)
    *   `eipp::List<T>`: elements are all of type T
*   tuple. (some elements, any type supported here)
    *   `eipp::Tuple<T1, T2...>`
*   map. (one key type, one value type. any type supported here)
    *   `eipp::Map<KeyType, ValueType>`


## Encode Example
```cpp
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

auto s = en.get_data();
std::ofstream f("/tmp/eippout");
f << s;
f.close();
```

```erlang
%% binary_to_term(D) will output:
{"Hello World!",101,
 {1,"Jim",[j1,j2]},
 [{2,"Tom",[t1,t2,t3]},{3,"David",[d1,d2,d3]}],
 #{<<"binary 1">> => [1,2,3],
   <<"binary 2">> => [4,5,6],
   <<"binary 3">> => [7,8,9]}}
```

## Decode Example

#### decode an integer

```erlang
%% Erlang
Value = 101.
Data = term_to_binary(Value).
```

```cpp
// read `Data` to char* buf

eipp::EIDecoder decoder(buf);
auto result = decoder.parse<eipp::Long>();
if(!decoder.is_valid()) {
    return -1;
}

std::cout << typeid(result).name() << ", " << result << std::endl;

// output:
// l, 101
```

#### decode a tuple with four simple elements
```erlang
%% Erlang
Value = {"v1string", <<"v2binary">>, 222, 1.23}.
Data = term_to_binary(Value).
```

```cpp
using T = eipp::Tuple<eipp::String, eipp::Binary, eipp::Long, eipp::Double >;

eipp::EIDecoder decoder(buf);
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

// outout
// NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, v1string
// NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, v2binary
// l, 222
// d, 1.23
```

#### decode a list which contains unknown size elements. elements are same type
```erlang
%% Erlang
Value = [{jack, 22}, {jim, 30}, {zoe, 28}, {john, 40}, {steve, 11}].
Data = term_to_binary(Value).
```

```cpp
using T = eipp::Tuple<eipp::Atom, eipp::Long>;
using T1 = eipp::List<T>;

eipp::EIDecoder decoder(buf);
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

// output
// NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, Jack
// l, 22
// NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, Jim
// l, 30
// NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, Zoe
// l, 28
// NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, John
// l, 40
// NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, Steve
// l, 11
```

#### decode a map, which value type is tuple
```erlang
%% Erlang
Value = #{1 => {1, 100, "One"}, 2 => {2, 200, "Two"}, 3 => {3, 300, "Three"}}.
Data = term_to_binary(Value).
```

```cpp
using T = eipp::Tuple<eipp::Long, eipp::Long, eipp::String>;
using T1 = eipp::Map<eipp::Long, T>;

eipp::EIDecoder decoder(buf);
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

// output
// l, 1
//     l, 1
//     l, 100
//     NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, One
// l, 2
//     l, 2
//     l, 200
//     NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, Two
// l, 3
//     l, 3
//     l, 300
//     NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, Three
```


[1]: http://erlang.org/doc/man/ei.html
[2]: http://erlang.org/doc/apps/erts/erl_ext_dist.html
