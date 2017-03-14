#ifndef EIPP_H
#define EIPP_H

#include <string>
#include <vector>
#include <map>
#include <type_traits>
#include <iterator>
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

    class _Base {
    public:
        virtual ~_Base(){}
        virtual int decode(const char* buf, int* index) = 0;
    };

    template <typename T, typename Decoder>
    class SingleType: public _Base {
    public:
        static const bool is_single = true;
        typedef T value_type;

        SingleType(): value(T()) {}

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
        T value;
    };

    template <typename Decoder>
    struct SingleTypeWithCharBuf: public _Base {
    public:
        static const bool is_single = true;
        typedef std::string value_type;

        SingleTypeWithCharBuf(): len(0) {}

        ~SingleTypeWithCharBuf() {
            delete[] char_ptr_value;
        }

        std::string& get_value() {
            return value;
        }

        int decode(const char* buf, int* index) override {
            return Decoder()(buf, index, this);
        }

    private:
        friend struct StringDecoder;
        friend struct BinaryDecoder;
        char * char_ptr_value;
        int len;
        std::string value;
    };

    struct LongDecoder {
        int operator ()(const char* buf, int* index, SingleType<long, LongDecoder>* s) {
            return ei_decode_long(buf, index, &(s->value));
        }
    };

    struct DoubleDecoder {
        int operator ()(const char* buf, int* index, SingleType<double, DoubleDecoder>* s) {
            return ei_decode_double(buf, index, &(s->value));
        }
    };

    struct StringDecoder {
        int operator ()(const char* buf, int* index, SingleTypeWithCharBuf<StringDecoder>* s) {
            int tp, ret;
            ret = ei_get_type(buf, index, &tp, &(s->len));
            if(ret == -1) return ret;

            s->char_ptr_value = new char[s->len+1]();
            ret = ei_decode_string(buf, index, s->char_ptr_value);
            if(ret == -1) return ret;

            s->value = std::string(s->char_ptr_value, (unsigned long)s->len);
            return ret;
        }
    };

    struct BinaryDecoder {
        int operator ()(const char* buf, int* index, SingleTypeWithCharBuf<BinaryDecoder>* s) {
            int tp, ret;
            ret = ei_get_type(buf, index, &tp, &(s->len));
            if(ret == -1) return ret;

            s->char_ptr_value = new char[s->len+1]();
            ret = ei_decode_binary(buf, index, s->char_ptr_value, (long *)&(s->len));
            if(ret == -1) return ret;

            s->value = std::string(s->char_ptr_value, (unsigned long)s->len);
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

    template <int(*_decode_header_func)(const char*, int*, int*), typename T, typename ... Types>
    class CompoundType: public _Base {
    public:
        static const bool is_single = false;
        typedef void value_type;    // not use, but should be here for std::conditional;

        CompoundType(): arity(0) {};
        ~CompoundType() {
            for(class _Base* ptr: value_ptr_vec) {
                delete ptr;
            }

            value_ptr_vec.clear();
        }

        size_t size() {
            return value_ptr_vec.size();
        }

        template <int index,
                typename = typename std::enable_if<TypeByIndex<index, T, Types...>::type::is_single>::type
                        >
        typename  TypeByIndex<index, T, Types...>::type::value_type get() {
            return dynamic_cast<typename TypeByIndex<index, T, Types...>::type*>(value_ptr_vec[index])->get_value();
        };

        template <int index,
                typename = typename std::enable_if<!TypeByIndex<index, T, Types...>::type::is_single>::type
        >
        typename  TypeByIndex<index, T, Types...>::type* get() {
            return dynamic_cast<typename TypeByIndex<index, T, Types...>::type*>(value_ptr_vec[index]);
        };

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

    protected:
        int arity;
        std::vector<class _Base*> value_ptr_vec;
    };


    template <typename T>
    class SoleTypeListType: public CompoundType<ei_decode_list_header, T> {
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

}

// simple type
using Long = detail::SingleType<long, detail::LongDecoder>;
using Double = detail::SingleType<double, detail::DoubleDecoder>;
using String = detail::SingleTypeWithCharBuf<detail::StringDecoder>;
using Binary = detail::SingleTypeWithCharBuf<detail::BinaryDecoder>;


// complex type
template <typename ... Types>
using Tuple = detail::CompoundType<ei_decode_tuple_header, Types...>;

template <typename T>
using List = detail::SoleTypeListType<T>;

template <typename ... Types>
using MultiTypeList = detail::CompoundType<ei_decode_list_header, Types...>;

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

}


#endif //EIPP_H
