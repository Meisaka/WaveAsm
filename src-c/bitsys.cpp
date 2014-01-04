
#include "bitsys.hpp"

BitString::BitString()
{
	cbits = 0;
	cbcount = 0;
}

BitString::~BitString()
{
	bits.clear();
}

std::string BitString::GetByteStringLE()
{
	std::string r;
	uint8_t byt;
	char nb[2];
	if(bits.size() > 0) {
	}
	for(int i = 0; i < cbcount; i+= 8) {
		byt = (uint8_t)(cbits >> i);
		nb[1] = '0' + (byt & 0x0f);
		nb[0] = '0' + ((byt >> 4) & 0x0f);
		if(nb[0] > '9') { nb[0] += 'A' - ('0' + 10); }
		if(nb[1] > '9') { nb[1] += 'A' - ('0' + 10); }
		r.append(nb,2);
	}
	return r;
}

std::string BitString::GetByteStringBE()
{
	std::string r;
	std::string s;
	uint8_t byt;
	char nb[2];
	bitstore_t cx;
	if(bits.size() > 0) {
		std::deque<bitstore_t>::iterator it = bits.begin();
		while(it != bits.end()) {
			cx = *(it++);
			s.clear();
			for(int i = 0; i < 32; i+= 8) {
				byt = (uint8_t)(cx >> i);
				nb[1] = '0' + (byt & 0x0f);
				nb[0] = '0' + ((byt >> 4) & 0x0f);
				if(nb[0] > '9') { nb[0] += 'A' - ('0' + 10); }
				if(nb[1] > '9') { nb[1] += 'A' - ('0' + 10); }
				s.insert(0,nb,2);
			}
			r += s;
		}
	}
	s.clear();
	for(int i = 0; i < cbcount; i+= 8) {
		byt = (uint8_t)(cbits >> i);
		nb[1] = '0' + (byt & 0x0f);
		nb[0] = '0' + ((byt >> 4) & 0x0f);
		if(nb[0] > '9') { nb[0] += 'A' - ('0' + 10); }
		if(nb[1] > '9') { nb[1] += 'A' - ('0' + 10); }
		s.insert(0,nb,2);
	}
	r += s;
	return r;
}

void BitString::AddRight(unsigned long v, int c)
{
	if(cbcount + c <= 8*sizeof(bitstore_t)) {
		v &= 0xFFFFffff >> (32 - c);
		cbits = (cbits << c) | v; 
		cbcount += c;
	} else {
		if(cbcount == 32) {
			bits.push_back(cbits);
			v &= 0xFFFFffff >> (32 - c);
			cbcount = c; cbits = v;
			return;
		} else { // run over
			int x = 32 - cbcount;
			bitstore_t t = (v >> x) & (0xFFFFffff >> (32 - x));
			cbits = (cbits << x) | t;
			c -= x;
			bits.push_back(cbits);
			v &= 0xFFFFffff >> (32 - c);
			cbcount = c; cbits = v;
		}
	}
}

void BitString::AddLeft(unsigned long v, int c)
{
	if(cbcount + c <= 8*sizeof(bitstore_t)) {
		v &= 0xFFFFffff >> (32 - c);
		cbits = (cbits >> c) | (v << (32 - c)); 
		cbcount += c;
	}
}
