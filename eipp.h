#ifndef EIPP_H
#define EIPP_H

#include <string>
#include <vector>
#include <map>
#include <stack>
#include <tuple>
#include <type_traits>
#include <iterator>
#include <ei.h>

namespace eipp {

enum class TYPE {
    Integer,
    Float,
    String,
    Binary,
    List,
    MultiTypeList,
    Tuple,
    Map
};


namespace detail {
    template <int index, typename Head, typename ... Tail>
    struct TypeByIndex {
        typedef typename TypeByIndex<index-1, Tail...>::type type;
    };

    template <typename Head, typename ... Tail>
    struct TypeByIndex<0, Head, Tail...> {
        typedef Head type;
    };

    class _Base {
    public:
        virtual ~_Base(){}
        virtual int decode(const char* buf, int* index) = 0;
    };

    template <TYPE tp, typename T, typename Decoder>
    class SingleType: public _Base {
    public:
        static const TYPE category_type = tp;
        static const bool is_single = true;
        typedef SingleType<tp, T, Decoder> self_type;
        typedef T value_type;

        SingleType(): value(T()) {}
//        SingleType(const T& v): value(v) {}
//        SingleType(T&& v): value(std::move(v)) {}
//
//        SingleType(const self_type& rhs) {
//            value = rhs.value;
//            std::cout << static_cast<std::underlying_type<TYPE>::type>(tp) << " Copy Constructor" << std::endl;
//        }
//
//        SingleType(self_type&& rhs) {
//            value = std::move(rhs.value);
//            std::cout << static_cast<std::underlying_type<TYPE>::type>(tp) << " Move Constructor" << std::endl;
//        }
//
//        SingleType&operator = (const self_type& rhs) {
//            value = rhs.value;
//            std::cout << static_cast<std::underlying_type<TYPE>::type>(tp) << " Assign Constructor" << std::endl;
//            return *this;
//        }
//
//        SingleType&operator = (self_type&& rhs) {
//            value = std::move(rhs.value);
//            std::cout << static_cast<std::underlying_type<TYPE>::type>(tp) << " Assign Move Constructor" << std::endl;
//            return *this;
//        }

        ~SingleType() {
        }

        T& get_value() {
            return value;
        }

        int decode(const char* buf, int* index) override {
            return Decoder()(buf, index, this);
        }

    private:
        friend struct LongDecoder;
        friend struct DoubleDecoder;
        friend struct StringDecoder;
        friend struct BinaryDecoder;
        T value;
    };


    struct LongDecoder {
        int operator ()(const char* buf, int* index, SingleType<TYPE::Integer, long, LongDecoder>* s) {
            return ei_decode_long(buf, index, &(s->value));
        }
    };

    struct DoubleDecoder {
        int operator ()(const char* buf, int* index, SingleType<TYPE::Float, double, DoubleDecoder>* s) {
            return ei_decode_double(buf, index, &(s->value));
        }
    };

    struct StringDecoder {
        int operator ()(const char* buf, int* index, SingleType<TYPE::String, std::string, StringDecoder>* s) {
            int tp=0, len=0, ret=0;
            ret = ei_get_type(buf, index, &tp, &len);
            if(ret == -1) return ret;

            char* ptr = new char[len+1]();
            ret = ei_decode_string(buf, index, ptr);
            if(ret == -1) return ret;

            s->value = std::string(ptr, (unsigned long)len);
            delete[] ptr;
            return ret;
        }
    };

    struct BinaryDecoder {
        int operator ()(const char* buf, int* index, SingleType<TYPE::Binary, std::string, BinaryDecoder>* s) {
            int tp=0, len=0, ret=0;
            ret = ei_get_type(buf, index, &tp, &len);
            if(ret == -1) return ret;

            char* ptr = new char[len+1]();
            ret = ei_decode_binary(buf, index, ptr, (long*)&ret);
            if(ret == -1) return ret;

            s->value = std::string(ptr, (unsigned long)len);
            delete[] ptr;
            return ret;
        }
    };


    template <typename ... Ts>
    int compound_decoder(const char* buf, int* index, std::vector<class _Base*>*);

    template <typename T, typename ... Ts>
    int compound_decoder_helper(const char* buf, int* index, std::vector<class _Base*>* vec) {
        class _Base* t = new T();
        int ret = t->decode(buf, index);
        vec->push_back(t);

        if(ret==-1) {
            return ret;
        } else {
            return compound_decoder<Ts...>(buf, index, vec);
        }
    };

    template <typename ... Ts>
    int compound_decoder(const char* buf, int* index, std::vector<class _Base*>* vec) {
        return compound_decoder_helper<Ts...>(buf, index, vec);
    }

    template <>
    int compound_decoder<>(const char*, int*, std::vector<class _Base*>*) {
        return 0;
    }

    template <TYPE tp, int(*_decode_header_func)(const char*, int*, int*), typename T, typename ... Types>
    class CompoundType: public _Base {
    public:
        static const TYPE category_type = tp;
        static const bool is_single = false;
        typedef CompoundType<tp, _decode_header_func, T, Types...> self_type;
        typedef void value_type;    // not use, but should be here for std::conditional;

        CompoundType(): arity(0) {}

//        CompoundType(const self_type& rhs) {
//            arity = rhs.arity;
//            value_ptr_vec = rhs.value_ptr_vec;
//            encode_value_tuple_ = rhs.encode_value_tuple_;
//        }
//
//        CompoundType&operator = (const self_type& rhs) {
//            arity = rhs.arity;
//            value_ptr_vec = rhs.value_ptr_vec;
//            encode_value_tuple_ = rhs.encode_value_tuple_;
//            return *this;
//        }

        ~CompoundType() {
            for(class _Base* ptr: value_ptr_vec) {
                delete ptr;
            }

            value_ptr_vec.clear();
        }

        template <int index, typename ThisType = typename TypeByIndex<index, T, Types...>::type>
        typename std::enable_if<ThisType::is_single, typename ThisType::value_type>::type
        get() {
            return dynamic_cast<ThisType*>(value_ptr_vec[index])->get_value();
        };

        template <int index, typename ThisType = typename TypeByIndex<index, T, Types...>::type>
        typename std::enable_if<!ThisType::is_single, ThisType*>::type
        get() {
            return dynamic_cast<ThisType*>(value_ptr_vec[index]);
        };

        int decode(const char* buf, int* index) override {
            int ret = 0;
            ret = _decode_header_func(buf, index, &arity);
            if (ret == 0) {
                if(sizeof...(Types) == 0) {
                    for(int i=0; i<arity; i++) {
                        ret = compound_decoder<T>(buf, index, &value_ptr_vec);
                        if(ret == -1) return ret;
                    }
                } else {
                    ret = compound_decoder<T, Types...>(buf, index, &value_ptr_vec);
                }
            }

            return ret;
        }

    protected:
        int arity;
        std::vector<class _Base*> value_ptr_vec;
//        std::tuple<T, Types...> encode_value_tuple_;
    };


    template <typename T>
    class SoleTypeListType: public CompoundType<TYPE::List, ei_decode_list_header, T> {
    public:
        typedef std::vector<class _Base*>::iterator IterType;

        struct Iterator: public std::iterator<std::forward_iterator_tag, IterType> {
            IterType iter;
            Iterator(IterType i): iter(i) {}
            Iterator(const Iterator& rhs): iter(rhs.iter) {}
            Iterator&operator = (const Iterator& rhs) {
                iter = rhs.iter;
                return *this;
            }

            Iterator operator ++ () {
                ++iter;
                return *this;
            }

            Iterator operator ++ (int) {
                Iterator tmp = *this;
                ++iter;
                return tmp;
            }

            bool operator == (const Iterator& rhs) {
                return iter == rhs.iter;
            }

            bool operator != (const Iterator& rhs) {
                return iter != rhs.iter;
            }

            template <typename X=T>
            typename std::enable_if<X::is_single, typename X::value_type>::type
            operator *() {
                return dynamic_cast<X*>(*iter)->get_value();
            }

            template <typename X=T>
            typename std::enable_if<!X::is_single, X*>::type
            operator *() {
                return dynamic_cast<X*>(*iter);
            }

        };

        typedef Iterator iterator;
        iterator begin() {
            return Iterator(this->value_ptr_vec.begin());
        }

        iterator end() {
            return Iterator(this->value_ptr_vec.end());
        }
    };

    template <typename KT, typename VT,
            typename = typename std::enable_if<
                    std::is_base_of<_Base, KT>::value && std::is_base_of<_Base, VT>::value>::type
    >
    class MapType: public _Base {
    public:
        static const TYPE category_type = TYPE::Map;
        static const bool is_single = false;
        typedef void value_type;    // not use, but should be here for std::conditional;

        using KeyType = typename std::conditional<KT::is_single, typename KT::value_type, KT*>::type;
        using ValueType = typename std::conditional<VT::is_single, typename VT::value_type, VT*>::type;

        typedef typename std::map<KeyType, ValueType>::iterator iterator;

        MapType(): arity(0) {}
        ~MapType() {
            for(_Base* ptr: value_ptr_vec) {
                delete ptr;
            }

            value.clear();
            value_ptr_vec.clear();
        }

        iterator begin() {
            return value.begin();
        }

        iterator end() {
            return value.end();
        }

        int decode(const char* buf, int* index) override {
            int ret = 0;
            ret = ei_decode_map_header(buf, index, &arity);
            if(ret == -1) return ret;

            for(int i = 0; i<arity; i++) {
                KT* k = new KT();
                ret = k->decode(buf, index);
                if (ret == -1) return ret;

                VT* v = new VT();
                ret = v->decode(buf, index);
                if(ret == -1) return ret;

                value_ptr_vec.push_back(k);
                value_ptr_vec.push_back(v);
                add_to_value(k, v);
            }

            return ret;
        }

    private:
        int arity;
        std::map<KeyType, ValueType> value;
        std::vector<_Base*> value_ptr_vec;

        template <typename Kt = KT, typename Vt = VT>
        typename std::enable_if<Kt::is_single && Vt::is_single, int>::type
        add_to_value(KT* k, VT* v) {
            value[k->get_value()] = v->get_value();
            return 0;
        }

        template <typename Kt = KT, typename Vt = VT>
        typename std::enable_if<Kt::is_single && !Vt::is_single, Vt*>::type
        add_to_value(KT* k, VT* v) {
            value[k->get_value()] = v;
            return v;
        }

        template <typename Kt = KT, typename Vt = VT>
        typename std::enable_if<!Kt::is_single && Vt::is_single, Kt*>::type
        add_to_value(KT* k, VT* v) {
            value[k] = v->get_value();
            return k;
        }

        template <typename Kt = KT, typename Vt = VT>
        typename std::enable_if<!Kt::is_single && !Vt::is_single, void>::type
        add_to_value(KT* k, VT* v) {
            value[k] = v;
        }
    };


    template <typename... >
    struct __is_one_of_helper;

    template <typename T>
    struct __is_one_of_helper<T> {
        static constexpr bool value = false;
    };

    template <typename T, typename ... Targets>
    struct __is_one_of_helper<T, T, Targets...> {
        static constexpr bool value = true;
    };

    template <typename T, typename T1, typename ... Targets>
    struct __is_one_of_helper<T, T1, Targets...> {
        static constexpr bool value = __is_one_of_helper<T, Targets...>::value;
    };

    template <typename T, typename ... Targets>
    struct is_one_of {
        using TClear = typename std::conditional<
                std::is_pointer<T>::value,
                typename std::add_pointer<typename std::remove_cv<typename std::remove_pointer<T>::type>::type>::type,
                typename std::remove_cv<T>::type
                >::type;

        static constexpr bool value = __is_one_of_helper<TClear, Targets...>::value;
    };

}

// simple type
using Long = detail::SingleType<TYPE::Integer, long, detail::LongDecoder>;
using Double = detail::SingleType<TYPE::Float, double, detail::DoubleDecoder>;
using String = detail::SingleType<TYPE::String, std::string, detail::StringDecoder>;
using Binary = detail::SingleType<TYPE::Binary, std::string, detail::BinaryDecoder>;


// complex type
template <typename ... Types>
using Tuple = detail::CompoundType<TYPE::Tuple, ei_decode_tuple_header, Types...>;

template <typename T>
using List = detail::SoleTypeListType<T>;

template <typename ... Types>
using MultiTypeList = detail::CompoundType<TYPE::MultiTypeList, ei_decode_list_header, Types...>;

template <typename KT, typename VT>
using Map = detail::MapType<KT, VT>;

class EIDecoder {
public:
    EIDecoder(char* buf):
            index_(0), version_(0), buf_(buf) {
        ret_ = ei_decode_version(buf_, &index_, &version_);
    }

    EIDecoder(const EIDecoder&) = delete;
    EIDecoder&operator=(const EIDecoder&) = delete;
    EIDecoder(EIDecoder&&) = delete;

    ~EIDecoder() {
        for(class detail::_Base* ptr: value_ptrs_) {
            delete ptr;
        }
    }

    bool is_valid() const {
        return ret_ == 0;
    }


    template <typename T>
    typename std::enable_if<std::is_base_of<detail::_Base, T>::value && T::is_single, typename T::value_type>::type
    parse() {
        detail::_Base* t = new T();
        ret_ = t->decode(buf_, &index_);

        value_ptrs_.push_back(t);
        return dynamic_cast<T*>(t)->get_value();
    }


    template <typename T>
    typename std::enable_if<std::is_base_of<detail::_Base, T>::value && !T::is_single, T*>::type
    parse() {
        detail::_Base* t = new T();
        ret_ = t->decode(buf_, &index_);

        value_ptrs_.push_back(t);
        return dynamic_cast<T*>(t);
    }


private:
    int index_;
    int version_;
    int ret_;
    const char* buf_;
    std::vector<class detail::_Base*> value_ptrs_;

};

//
//class EIEncoder {
//public:
//
//    EIEncoder() {
//        base_buff_ = new ei_x_buff;
//        ei_x_new_with_version(base_buff_);
//        x_buff_stack_.push(base_buff_);
//    }
//
//    EIEncoder(const EIEncoder&) = delete;
//    EIEncoder&operator = (const EIEncoder&) = delete;
//    EIEncoder(EIEncoder&&) = delete;
//
//    ~EIEncoder() {
//        while(!x_buff_stack_.empty()) {
//            delete x_buff_stack_.top();
//            x_buff_stack_.pop();
//        }
//    }
//
//    void start_encode_list() {
//        ei_x_buff* sub_buff = new ei_x_buff;
//        x_buff_stack_.push(sub_buff);
//
//        incr_arity();
//        init_arity();
//    }
//
//    void end_encode_list() {
//        auto arity = get_arity();
//        auto this_buff = current_buff();
//        x_buff_stack_.pop();
//
//        auto prev_buff = current_buff();
//        ei_x_encode_list_header(prev_buff, arity);
//        ei_x_append(prev_buff, this_buff);
//        ei_x_encode_empty_list(prev_buff);
//
//        delete this_buff;
//    }
//
//    template <typename T>
//    typename std::enable_if<std::is_integral<T>::value &&
//            ! detail::is_one_of<T, long long, unsigned long long>::value>::type
//    encode(const T& arg) {
//        std::cout << "encode long" << std::endl;
//        ei_x_encode_long(current_buff(), (long)arg);
//        incr_arity();
//    }
//
//    template <typename T>
//    typename std::enable_if<detail::is_one_of<T, long long, unsigned long long>::value>::type
//    encode(const T& arg) {
//        std::cout << "encode long long" << std::endl;
//        ei_x_encode_longlong(current_buff(), (long long)arg);
//        incr_arity();
//    };
//
//    template <typename T>
//    typename std::enable_if<std::is_floating_point<T>::value>::type
//    encode(const T& arg) {
//        ei_x_encode_double(current_buff(), (double)arg);
//        incr_arity();
//    }
//
//    template <typename T>
//    typename std::enable_if<detail::is_one_of<T, char *, unsigned char *>::value>::type
//    encode(const T& arg, TYPE tp = TYPE::String) {
//        if (tp == TYPE::String) {
//            ei_x_encode_string(current_buff(), arg);
//        } else {
//            ei_x_encode_binary(current_buff(), arg, (int)strlen(arg));
//        }
//
//        incr_arity();
//    };
//
//    void
//    encode(const std::string& arg, TYPE tp = TYPE::String) {
//        if (tp == TYPE::String) {
//            ei_x_encode_string_len(current_buff(), arg.c_str(), (int)arg.length());
//        } else {
//            ei_x_encode_binary(current_buff(), arg.c_str(), (int)arg.length());
//        }
//
//        incr_arity();
//    }
//
//
//    std::string get_data() {
//        std::string s(base_buff_->buff, (unsigned long)base_buff_->index);
//        return s;
//    }
//
//private:
//    ei_x_buff* base_buff_;
//    std::stack<ei_x_buff*> x_buff_stack_;
//    std::stack<int> arity_stack_;   // for list, tuple, map
//
//    ei_x_buff* current_buff() {
//        return x_buff_stack_.top();
//    }
//
//    void init_arity() {
//        arity_stack_.push(0);
//    }
//
//    void incr_arity() {
//        if(!arity_stack_.empty()) {
//            arity_stack_.top()++;
//        }
//    }
//
//    int get_arity() {
//        int arity = arity_stack_.top();
//        arity_stack_.pop();
//        return arity;
//    }
//};


}


#endif //EIPP_H
