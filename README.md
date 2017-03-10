# EIPP

A C++11 Header Only Library for using erlang ei library.

It's a wrapper of `ei.h`.

The Goal of EIPP is interactive between Erlang system and other Language.

e.g. Write c++ driver for erlang, or encode/decode [External Term Format][1]
                                                   
The library **ONLY** support the following types:

*   long. (include char, short, int, long)
*   double. (include float, double)
*   string. (byte sequence, byte in 0-255)
*   binary. (byte sequence, any bytes)
*   list. (different or same type elements, any type supported here)
*   tuple. (some elements, any type supported here)
*   map. (one key type, one value type. any type supported here)


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
std::cout << result << std::endl;

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
// Pc, v1string
// Pc, v2binary
// l, 222
// d, 1.23
```

#### decode a list which contains unknown size elements. elements are same type
```erlang
%% Erlang
Value = [{"Jack", 22}, {"Jim", 30}, {"Zoe", 28}, {"John", 40}, {"Steve", 11}].
Data = term_to_binary(Value).
```

```cpp
using T = eipp::Tuple<eipp::String, eipp::Long>;
using T1 = eipp::List<T>;

eipp::EIDecoder decoder(buf);
auto result = decoder.parse<T1>();
if(!decoder.is_valid()) {
    return -1;
}

for (size_t i = 0; i < result->size(); i++) {
    auto tuple = result->get<>(i);

    auto v1 = tuple->get<0>();
    auto v2 = tuple->get<1>();

    std::cout << typeid(v1).name() << ", " << v1 << std::endl;
    std::cout << typeid(v2).name() << ", " << v2 << std::endl;
}

// output
// Pc, Jack
// l, 22
// Pc, Jim
// l, 30
// Pc, Zoe
// l, 28
// Pc, John
// l, 40
// Pc, Steve
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

// output
// l, 1
//     l, 1
//     l, 100
//     Pc, One
// l, 2
//     l, 2
//     l, 200
//     Pc, Two
// l, 3
//     l, 3
//     l, 300
//     Pc, Three
```

## TODO
1.  implement custom iterator
2.  EIEncoder


[1]: http://erlang.org/doc/apps/erts/erl_ext_dist.html
