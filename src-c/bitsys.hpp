
#include <string>
#include <deque>

typedef unsigned long bitstore_t;
typedef unsigned char uint8_t;

class BitString {
public:
	BitString();
	virtual ~BitString();

	void AddRight(bitstore_t v, int c);
	void AddLeft(bitstore_t v, int c);
	std::string GetByteStringLE();
	std::string GetByteStringBE();
#define BITSTRA(x) \
	BitString& operator<<(x l) { AddRight((bitstore_t)l, 8*sizeof(x)); return *this; } \
	BitString& operator>>(x l) { AddLeft((bitstore_t)l, 8*sizeof(x)); return *this; }
	BITSTRA(char)
	BITSTRA(short)
	BITSTRA(int)
	BITSTRA(long)
	BITSTRA(unsigned char)
	BITSTRA(unsigned short)
	BITSTRA(unsigned int)
	BITSTRA(unsigned long)
#undef BITSTRA
private:
	std::deque<bitstore_t> bits;
	bitstore_t cbits;
	int cbcount;
};

