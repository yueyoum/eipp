#ifndef EIPP_H
#define EIPP_H

#include <vector>
#include <map>
#include <type_traits>
#include <ei.h>

namespace eipp {

namespace detail {
    template <int index, typename Head, typename ... Tail>
    struct TypeByIndex {
        typedef typename TypeByIndex<index-1, Tail...>::type type;
    };

    template <typename Head, typename ... Tail>
    struct TypeByIndex<0, Head, Tail...> {
        typedef Head type;
    };

    struct _Base {
        virtual ~_Base(){}
        virtual int decode(const char* buf, int* index) = 0;
    };

    template <typename T, typename Decoder>
    struct SingleType: _Base {
        static const bool is_single = true;
        typedef T value_type;
        T value;

        SingleType() {
            value = T();
        }

        ~SingleType() {
        }

        int decode(const char* buf, int* index) override {
            return Decoder()(buf, index, this);
        }
    };

    template <typename Decoder>
    struct SingleTypeWithCharBuf: _Base {
        static const bool is_single = true;
        typedef char * value_type;
        char * value;
        int len;

        SingleTypeWithCharBuf() {
            len = 0;
        }

        ~SingleTypeWithCharBuf() {
            delete[] value;
        }

        int decode(const char* buf, int* index) override {
            return Decoder()(buf, index, this);
        }
    };

    template <typename T, int(*_decode_func)(const char*, int*, T*)>
    struct SimpleDecoder {
        int operator ()(const char* buf, int* index, SingleType<T, SimpleDecoder<T, _decode_func>>* s) {
            return _decode_func(buf, index, &(s->value));
        }
    };

    using LongDecoder = SimpleDecoder<long, ei_decode_long>;
    using DoubleDecoder = SimpleDecoder<double, ei_decode_double>;

    struct StringDecoder {
        int operator ()(const char* buf, int* index, SingleTypeWithCharBuf<StringDecoder>* s) {
            int tp, ret;
            ret = ei_get_type(buf, index, &tp, &(s->len));
            if(ret == -1) return ret;

            s->value = new char[s->len+1]{'\0'};
            return ei_decode_string(buf, index, s->value);
        }
    };

    struct BinaryDecoder {
        int operator ()(const char* buf, int* index, SingleTypeWithCharBuf<BinaryDecoder>* s) {
            int tp, ret;
            ret = ei_get_type(buf, index, &tp, &(s->len));
            if(ret == -1) return ret;

            s->value = new char[s->len+1]{'\0'};
            return ei_decode_binary(buf, index, s->value, (long *)&(s->len));
        }
    };


    template <typename ... Ts>
    int compound_decoder(const char* buf, int* index, std::vector<struct _Base*>*);

    template <typename T, typename ... Ts>
    int compound_decoder_helper(const char* buf, int* index, std::vector<struct _Base*>* vec) {
        struct _Base* t = new T();
        int ret = t->decode(buf, index);
        vec->push_back(t);

        if(ret==-1) {
            return ret;
        } else {
            return compound_decoder<Ts...>(buf, index, vec);
        }
    };

    template <typename ... Ts>
    int compound_decoder(const char* buf, int* index, std::vector<struct _Base*>* vec) {
        return compound_decoder_helper<Ts...>(buf, index, vec);
    }

    template <>
    int compound_decoder<>(const char*, int*, std::vector<struct _Base*>*) {
        return 0;
    }

    template <int(*_decode_header_func)(const char*, int*, int*), typename T, typename ... Types>
    struct CompoundType: _Base {
        static const bool is_single = false;
        typedef void value_type;    // not use, but should be here for std::conditional;
        int arity;
        std::vector<struct _Base*> value_ptr_vec;

        CompoundType(): arity(0) {};
        ~CompoundType() {
            for(struct _Base* ptr: value_ptr_vec) {
                delete ptr;
            }

            value_ptr_vec.clear();
        }

        size_t size() {
            return value_ptr_vec.size();
        }

        // get<N>();
        // mostly used for get value of known struct tuple
        // or used for known length small list which contains different type elements
        // e.g. eipp::Tuple<eipp::Long, eipp::Double>
        // e.g. eipp::List<eipp::Long, eipp::Tuple<...>>
        template <int index,
                typename = typename std::enable_if<TypeByIndex<index, T, Types...>::type::is_single>::type
                        >
        typename  TypeByIndex<index, T, Types...>::type::value_type get() {
            return dynamic_cast<typename TypeByIndex<index, T, Types...>::type*>(value_ptr_vec[index])->value;
        };

        template <int index,
                typename = typename std::enable_if<!TypeByIndex<index, T, Types...>::type::is_single>::type
        >
        typename  TypeByIndex<index, T, Types...>::type* get() {
            return dynamic_cast<typename TypeByIndex<index, T, Types...>::type*>(value_ptr_vec[index]);
        };

        // get<>(N);
        // only used for list which contains SAME type elements
        // e.g. eipp::List<eipp::Long>
        // e.g. eipp::List<eipp::Tuple<...>>
        template <typename T1=T, typename = typename std::enable_if<std::is_same<T1, T>::value && T1::is_single>::type>
        typename T1::value_type get(int index) {
            return dynamic_cast<T1*>(value_ptr_vec[index])->value;
        }

        template <typename T1=T, typename = typename std::enable_if<std::is_same<T1, T>::value && !T1::is_single>::type>
        T1* get(int index) {
            return dynamic_cast<T1*>(value_ptr_vec[index]);
        }

        int decode(const char* buf, int* index) override {
            int ret = 0;
            ret = _decode_header_func(buf, index, &arity);
            if (ret == -1) {
                return ret;
            } else {
                if(sizeof...(Types) == 0) {
                    for(int i=0; i<arity; i++) {
                        ret = compound_decoder<T>(buf, index, &value_ptr_vec);
                        if(ret == -1) return ret;
                    }
                    return ret;

                } else {
                    return compound_decoder<T, Types...>(buf, index, &value_ptr_vec);
                }
            }
        }

    };


    template <typename KT, typename VT,
            typename = typename std::enable_if<
                    std::is_base_of<_Base, KT>::value && std::is_base_of<_Base, VT>::value>::type
    >
    struct MapType: _Base {
        static const bool is_single = false;
        typedef void value_type;    // not use, but should be here for std::conditional;
        int arity;

        using KeyType = typename std::conditional<KT::is_single, typename KT::value_type, KT*>::type;
        using ValueType = typename std::conditional<VT::is_single, typename VT::value_type, VT*>::type;

        std::map<KeyType, ValueType> value;
        std::vector<_Base*> value_ptr_vec;

        MapType(): arity(0) {}
        ~MapType() {
            for(_Base* ptr: value_ptr_vec) {
                delete ptr;
            }

            value.clear();
            value_ptr_vec.clear();
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
                _add_to_value<>(k, v);
            }

            return ret;
        }

        template <typename Kt = KT, typename Vt = VT,
                typename = typename std::enable_if<Kt::is_single && Vt::is_single>::type
                        >
        void _add_to_value(KT* k, VT* v) {
            value[k->value] = v->value;
        }

        template <typename Kt = KT, typename Vt = VT,
                typename = typename std::enable_if<Kt::is_single && !Vt::is_single>::type
        >
        Vt* _add_to_value(KT* k, VT* v) {
            value[k->value] = v;
            return v;
        }

        template <typename Kt = KT, typename Vt = VT,
                typename = typename std::enable_if<!Kt::is_single && Vt::is_single>::type
        >
        Kt* _add_to_value(KT* k, VT* v) {
            value[k] = v->value;
            return k;
        }

        template <typename Kt = KT, typename Vt = VT,
                typename = typename std::enable_if<!Kt::is_single && !Vt::is_single>::type
        >
        int _add_to_value(KT* k, VT* v) {
            value[k->value] = v->value;
            return 0;
        }

    };

}

// simple type
using Long = detail::SingleType<long, detail::LongDecoder>;
using Double = detail::SingleType<double, detail::DoubleDecoder>;
using String = detail::SingleTypeWithCharBuf<detail::StringDecoder>;
using Binary = detail::SingleTypeWithCharBuf<detail::BinaryDecoder>;


// complex type
template <typename ... Types>
using Tuple = detail::CompoundType<ei_decode_tuple_header, Types...>;

template <typename ... Types>
using List = detail::CompoundType<ei_decode_list_header, Types...>;

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

    virtual ~EIDecoder() {
        for(struct detail::_Base* ptr: compound_ptrs_) {
            delete ptr;
        }
    }

    bool is_valid() const {
        return ret_ == 0;
    }

    template <typename T,
            typename = typename std::enable_if<
                    std::is_base_of<detail::_Base, T>::value && T::is_single
            >::type>
    typename T::value_type parse() {
        detail::_Base* t = new T();
        ret_ = t->decode(buf_, &index_);

        compound_ptrs_.push_back(t);
        return dynamic_cast<T*>(t)->value;
    }

    template <typename T,
            typename = typename std::enable_if<
                    std::is_base_of<detail::_Base, T>::value && !T::is_single
            >::type>
    T* parse() {
        detail::_Base* t = new T();
        ret_ = t->decode(buf_, &index_);

        compound_ptrs_.push_back(t);
        return dynamic_cast<T*>(t);
    }


private:
    int index_;
    int version_;
    int ret_;
    const char* buf_;
    std::vector<struct detail::_Base*> compound_ptrs_;

};

}


#endif //EIPP_H
