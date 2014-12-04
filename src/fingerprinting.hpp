#include <cstdlib>

template <int kgrams>
class RollingHash {
		
};

template <int kgrams, typename hasher = RollingHash<kgrams> >
class Fingerprinter {
	
};