#ifndef BASIC_FIELD
#define BASIC_FIELD

#include <cstring>

/**
Basic Field:
N: base of field
key_type: type of object to be stored in field
key_bits: length of object to be stored in bits
Example: N=2, key_bits= 8, would correspond to all things in 2^8 (so characters)
**/
template<int N, int key_bits>
class Field {
  public:
  	uint8_t args[key_bits] = {};
    
    Field()	{
        assert( N <= (1 << sizeof(uint8_t)*8 ));
    }

    void add( std::string* key) {
        add(key->c_str());
    }

    void add( const void* key ) {
    	add_n_times( key, 1);
    }

    void add( const Field<N, key_bits> field_elt) {
        for(int i = 0; i < key_bits; ++i) {
            args[i] = field_add(args[i], field_elt.args[i]);
        }
    }

    void remove( std::string* key) {
        remove(key->c_str());
    }

    void remove(const void* key) {
    	add_n_times( key, -1);
    }

    void remove(const Field<N, key_bits> field_elt) {
        for(int i = 0; i < key_bits; ++i) {
            args[i] = field_add(args[i], (-1)*field_elt.args[i]);
        }
    }

    void add_n_times( const void* key, int times) {
    	for(int i = 0; i < key_bits; ++i ) {
    		args[i] = field_add(args[i], times*bit_is_set(key, i));
    	}
    }

    //checks if all "nits" can be evely divided by n
    bool can_divide_by(int n) {
        assert(n != 0);
        for(int i = 0; i < key_bits; ++i) {
            if( !can_field_divide(args[i], n)) 
                return false;
        }
        return true;
    }
    
    bool extract_key( std::string* key) {
        char buf[key_bits/8] = {};
        bool res = extract_key(buf);
        key->assign(buf);
        return res;
    }

    //returns true if extractable key exists (all bits are <=1), false otherwise
    bool extract_key( void* key) {
        if( !can_divide_by(1) )
            return false;
    	extract_key( key, 1);
        return true;
    }

    void extract_key( std::string* key, int n) {
        char buf[key_bits/8] = {};
        extract_key( buf, n);
        key->assign(buf);
    }

    //assumes that key is extractable. key should appear n times
    void extract_key( void* key, int n) {
        memset(key, 0, key_bits/8);
        assert( can_divide_by(n) );  
        for(int i = 0; i < key_bits; ++i ) {
            copy_bit(key, i);
        }
    }

    // bool can_extract() {
    //     for(int i = 0; i < key_bits; ++i ) {
    //         if( args[i] > 1 ) 
    //             return false;
    //     }
    //     return true;
    // }

    void print_contents() const {
    	for(int i = 0; i < key_bits; ++i) {
    		printf("%d", args[i]);
    	}
    	printf("\n");
    }

    bool operator==( const Field<N, key_bits>& other ) const {
        for(int i = 0; i < key_bits; ++i) {
            if(args[i] != other.args[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator<( const Field<N, key_bits>& other ) const {
        for(int i = 0; i < key_bits; ++i) {
            if(args[i] < other.args[i]) {
                return true;
            }
        }
        return false;
    }

//PRIVATE
    //TODO: be careful of overflow?
    inline uint8_t field_add(int x, int y) {
        return ((x + y) % N + N) % N;
    }

    inline uint8_t field_multiply(int x, int y) {
        return ((x*y) % N + N) % N;
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

    inline bool bit_is_set(const void* key, int i) {
        return ( (((uint8_t*) key)[i/8] & (1 << (i % 8))) != 0 );
    }

    //sets the ith bit of the key to 1 if the ith nit of our field is nonzero
    //sets it to 0 otherwise
    inline void copy_bit(void* key, int i) {
        ((uint8_t*) key)[i/8] |= ((args[i] != 0) << (i % 8));
    }
};

#endif