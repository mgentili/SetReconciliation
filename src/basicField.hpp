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

    bool is_empty() const {
        return( arg == 0);
    }

    void print_contents() const {
        printf("%d\n", arg);
    }
};

template<int N, int key_bits, typename chunk_type = uint8_t>
class BaseField {
  public:
    chunk_type arg[key_bits] = {};
    BaseField() {
        assert( N <= (1 << sizeof(chunk_type)*8 ));
        //assert( key_bits % 8 == 0 && key_bits >= 8);
    }

    bool is_empty() const {
        for(int i = 0; i < key_bits; ++i) {
            if( arg[i] != 0 ) {
                return false;
            }
        }
        return true;
    }

    // void add( const BaseField<N, key_bits> field_elt) {
    //     for(int i = 0; i < key_bits; ++i) {
    //         arg[i] = field_add(arg[i], field_elt.arg[i]);
    //     }
    // }

    // void remove(const BaseField<N, key_bits> field_elt) {
    //     for(int i = 0; i < key_bits; ++i) {
    //         arg[i] = field_add(arg[i], (-1)*field_elt.arg[i]);
    //     }
    // }

    //checks if all "nits" can be evenly divided by n
    bool can_divide_by(int n) {
        assert(n != 0);
        for(int i = 0; i < key_bits; ++i) {
            if( !can_field_divide(arg[i], n)) 
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
            printf("%d", arg[i]);
        }
        printf("\n");
    }

    bool operator==( const BaseField<N, key_bits>& other ) const {
        for(int i = 0; i < key_bits; ++i) {
            if(arg[i] != other.arg[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator<( const BaseField<N, key_bits>& other ) const {
        for(int i = 0; i < key_bits; ++i) {
            if(arg[i] < other.arg[i]) {
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
    using BaseField<N, key_bits>::arg;
    using BaseField<N, key_bits>::field_add;

    void add( const key_type& key) {
        add_n_times( key, 1);   
    }

    void add( const Field<N, key_type, key_bits> field_elt) {
        for(int i = 0; i < key_bits; ++i) {
            arg[i] = field_add(arg[i], field_elt.arg[i]);
        }
    }

    void remove( const key_type& key) {
        add_n_times( key, -1);
    }

    void remove( const Field<N, key_type, key_bits> field_elt) {
        for(int i = 0; i < key_bits; ++i) {
            arg[i] = field_add(arg[i], (-1)*field_elt.arg[i]);
        }
    }

    void add_n_times( const key_type& key, int n) {
        for(int i = 0; i < key_bits; ++i) {
            arg[i] = field_add(arg[i], n*bit_is_set(key, i) );
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
        uint64_t x = (arg[i] != 0);
        key |= (x << i);
    }
};

template <int N, int key_bits>
class Field<N, std::string, key_bits> : public BaseField<N, key_bits> {
  public:
    using BaseField<N, key_bits>::arg;
    using BaseField<N, key_bits>::field_add;
    void add( const std::string& key) {
        add_n_times(key.c_str(), 1);
    }
    
    void add( const Field<N, std::string, key_bits> field_elt) {
        for(int i = 0; i < key_bits; ++i) {
            arg[i] = field_add(arg[i], field_elt.arg[i]);
        }
    }

    void remove( const std::string& key) {
        add_n_times(key.c_str(), -1);
    }

    void remove( const Field<N, std::string, key_bits> field_elt) {
        for(int i = 0; i < key_bits; ++i) {
            arg[i] = field_add(arg[i], (-1)*field_elt.arg[i]);
        }
    }

    void add_n_times( const char* key, int n) {
        for(int i = 0; i < key_bits; ++i) {
            arg[i] = field_add(arg[i], n*bit_is_set(key, i) );
        }
    }

    inline bool bit_is_set(const char* key, int i) {
        return ( (((uint8_t*) key)[i/8] & (1 << (i % 8))) != 0 );
    }

    //sets the ith bit of the key to 1 if the ith nit of our field is nonzero
    //sets it to 0 otherwise
    inline void copy_bit(char* key, int i) {
        ((uint8_t*) key)[i/8] |= ((arg[i] != 0) << (i % 8));
    }

    void extract_key( std::string& key) {
        char buf[key_bits/8] = {};
        for(int i = 0; i < key_bits; ++i) {
            copy_bit(buf, i);
        }
        key.assign(buf, key_bits/8);
    }
};

template<typename key_type, int key_bits>
class Field<2, key_type, key_bits> {
  public:
    key_type arg = 0;

    Field() {
        assert( key_bits <= sizeof(uint64_t)*8);
    }

    void add( const key_type& key ) {
        arg ^= key;
    }

    void add( const Field<2, key_type, key_bits> field_elt ) {
        arg ^= field_elt.arg;
    }

    void remove(const key_type& key) {
        add(key);
    }

    void remove( const Field<2, key_type, key_bits> field_elt ) {
        add(field_elt);
    }

    void extract_key( key_type& key) {
        key = (key_type) arg;
    }

    bool can_divide_by(int n) {
        return true;
    }

    void print_contents() const {
        printf("%lu\n", arg);
    }

    bool is_empty() const {
        return (arg == 0);
    }

    bool operator==( const Field<2, key_type, key_bits>& other ) const {
        return arg == other.arg;
    }

    bool operator<( const Field<2, key_type, key_bits>& other ) const {
        return arg < other.arg;
    }
};

template<int key_bits>
class Field<2, std::string, key_bits> {
  public:
    char arg[key_bits/8] = {};

    Field() {}

    void add( const std::string& key ) {
        for(int i = 0; i < key_bits/8; ++i) {
            arg[i] ^= key[i];
        }
    }

    void add( const Field<2, std::string, key_bits> field_elt ) {
        for(int i = 0; i < key_bits/8; ++i) {
            arg[i] ^= field_elt.arg[i];
        }
    }

    void remove( const std::string& key) {
        add(key);
    }

    void remove( const Field<2, std::string, key_bits> field_elt ) {
        add(field_elt);
    }

    void extract_key( std::string& key) {
        // char buf[key_bits/8] = {};
        // for(int i = 0; i < key_bits/8; ++i) {
        //     copy_char(buf, i);
        // }
        key.assign(arg, key_bits/8);
    }

    bool can_divide_by(int n) {
        return true;
    }

    bool is_empty() const {
        for(int i = 0; i < key_bits/8; ++i) {
            if(arg[i] != 0) {
                return false;
            }
        }
        return true;
    }

    void print_contents() const {
        for(int i = 0; i < key_bits/8; ++i) {
            for(int j = 0; j < 8; ++j) {
                printf("%d", (arg[i] & (1 << j)) != 0);
            }
        }
        printf("\n");
    }

    bool operator==( const Field<2, std::string, key_bits>& other ) const {
        for(int i = 0; i < key_bits/8; ++i) {
            if(arg[i] != other.arg[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator<( const Field<2, std::string, key_bits>& other ) const {
        for(int i = 0; i < key_bits/8; ++i) {
            if(arg[i] < other.arg[i]) {
                return true;
            }
        }
        return false;
    }
};

#endif 