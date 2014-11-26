#ifndef _BASIC_FIELD
#define _BASIC_FIELD

#include <cstring>

//TODO: Figure out how to make template stuff work better?

template<int N>
class SimpleField {
  public:
    int arg = 0;
    SimpleField() {}

    void add( int x) {
        arg = ((arg + x) % N + N) % N;
    }

    void add( const SimpleField<N> field_elt ) {
        add(field_elt.get_contents());
    }

    void remove( int x) {
        add(-x);
    }

    void remove( const SimpleField<N> field_elt ) {
        remove(field_elt.get_contents());
    }

    int get_contents() const {
        return arg;
    }

    void print_contents() const {
        printf("%d\n", arg);
    }
};

template<int N, int key_bits>
class BaseField {
  public:
    uint8_t args[key_bits] = {};
    BaseField() {
        assert( N <= (1 << sizeof(uint8_t)*8 ));
        //assert( key_bits % 8 == 0 && key_bits >= 8);
    }

    // void add( const BaseField<N, key_bits> field_elt) {
    //     for(int i = 0; i < key_bits; ++i) {
    //         args[i] = field_add(args[i], field_elt.args[i]);
    //     }
    // }

    // void remove(const BaseField<N, key_bits> field_elt) {
    //     for(int i = 0; i < key_bits; ++i) {
    //         args[i] = field_add(args[i], (-1)*field_elt.args[i]);
    //     }
    // }

    //checks if all "nits" can be evenly divided by n
    bool can_divide_by(int n) {
        assert(n != 0);
        for(int i = 0; i < key_bits; ++i) {
            if( !can_field_divide(args[i], n)) 
                return false;
        }
        return true;
    }

    inline uint8_t field_add(int x, int y) {
        return ((x + y) % N + N) % N;
    }

    inline uint8_t field_multiply(int x, int y) {
        return ((x*y) % N + N) % N;
    }

    void print_contents() const {
        for(int i = 0; i < key_bits; ++i) {
            printf("%d", args[i]);
        }
        printf("\n");
    }

    bool operator==( const BaseField<N, key_bits>& other ) const {
        for(int i = 0; i < key_bits; ++i) {
            if(args[i] != other.args[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator<( const BaseField<N, key_bits>& other ) const {
        for(int i = 0; i < key_bits; ++i) {
            if(args[i] < other.args[i]) {
                return true;
            }
        }
        return false;
    }

    //TODO: not exactly the right definition
    inline bool can_field_divide(int x, int y) {
        if( x == 0)
            return true;
        if( y < 0) {
            x = field_multiply(x, -1);
            y = -y;
        }
        return x == y;
    }
};

template<int N, typename key_type, int key_bits = 8*sizeof(key_type)>
class Field : public BaseField<N, key_bits> {
  public: 
    using BaseField<N, key_bits>::args;
    using BaseField<N, key_bits>::field_add;

    void add( const key_type& key) {
        add_n_times( key, 1);   
    }

    void add( const Field<N, key_type, key_bits> field_elt) {
        for(int i = 0; i < key_bits; ++i) {
            args[i] = field_add(args[i], field_elt.args[i]);
        }
    }

    void remove( const key_type& key) {
        add_n_times( key, -1);
    }

    void remove( const Field<N, key_type, key_bits> field_elt) {
        for(int i = 0; i < key_bits; ++i) {
            args[i] = field_add(args[i], (-1)*field_elt.args[i]);
        }
    }

    void add_n_times( const key_type& key, int n) {
        for(int i = 0; i < key_bits; ++i) {
            args[i] = field_add(args[i], n*bit_is_set(key, i) );
        }
    }

    //precondition: can_divide_by(n) succeeded for some n
    //meaning that there is *potentially* a single key duplicated
    //multiple times within
    void extract_key( key_type& key ) {
        key = 0;
        for(int i = 0; i < key_bits; ++i) {
            copy_bit(key, i);
        }
    }

    inline bool bit_is_set( const key_type& key, int i) {
        return( (key & (1ULL << i)) != 0 );
    }

    inline void copy_bit( key_type& key, int i) {
        uint64_t x = (args[i] != 0);
        key |= (x << i);
    }
};

template <int N, int key_bits>
class Field<N, std::string, key_bits> : public BaseField<N, key_bits> {
  public:
    using BaseField<N, key_bits>::args;
    using BaseField<N, key_bits>::field_add;
    void add( const std::string& key) {
        add_n_times(key.c_str(), 1);
    }
    
    void add( const Field<N, std::string, key_bits> field_elt) {
        for(int i = 0; i < key_bits; ++i) {
            args[i] = field_add(args[i], field_elt.args[i]);
        }
    }

    void remove( const std::string& key) {
        add_n_times(key.c_str(), -1);
    }

    void remove( const Field<N, std::string, key_bits> field_elt) {
        for(int i = 0; i < key_bits; ++i) {
            args[i] = field_add(args[i], (-1)*field_elt.args[i]);
        }
    }

    void add_n_times( const char* key, int n) {
        for(int i = 0; i < key_bits; ++i) {
            args[i] = field_add(args[i], n*bit_is_set(key, i) );
        }
    }

    inline bool bit_is_set(const char* key, int i) {
        return ( (((uint8_t*) key)[i/8] & (1 << (i % 8))) != 0 );
    }

    //sets the ith bit of the key to 1 if the ith nit of our field is nonzero
    //sets it to 0 otherwise
    inline void copy_bit(char* key, int i) {
        ((uint8_t*) key)[i/8] |= ((args[i] != 0) << (i % 8));
    }

    void extract_key( std::string& key) {
        char buf[key_bits/8] = {};
        for(int i = 0; i < key_bits; ++i) {
            copy_bit(buf, i);
        }
        key.assign(buf, key_bits/8);
    }
};

#endif