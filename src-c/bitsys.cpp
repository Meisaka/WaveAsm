
#include "bitsys.hpp"

BitString::BitString()
{
	rbits = 0;
	rbcount = 0;
	lbits = 0;
	lbcount = 0;
}

BitString::~BitString()
{
	bits.clear();
}

void BitString::clear()
{
	bits.clear();
	rbits = 0;
	rbcount = 0;
	lbits = 0;
	lbcount = 0;
}

std::string BitString::GetHexLE()
{
	std::string r;
	std::string s;
	uint8_t byt;
	char nb[2];
	bitstore_t cx;
	s.clear();
	for(int i = 0; i < lbcount; i+= 8) {
		byt = (uint8_t)(lbits >> i);
		nb[1] = '0' + (byt & 0x0f);
		nb[0] = '0' + ((byt >> 4) & 0x0f);
		if(nb[0] > '9') { nb[0] += 'A' - ('0' + 10); }
		if(nb[1] > '9') { nb[1] += 'A' - ('0' + 10); }
		s.append(nb,2);
	}
	r += s;
	if(bits.size() > 0) {
		std::deque<bitstore_t>::reverse_iterator it = bits.rbegin();
		while(it != bits.rend()) {
			cx = *(it++);
			s.clear();
			for(int i = 0; i < 32; i+= 8) {
				byt = (uint8_t)(cx >> i);
				nb[1] = '0' + (byt & 0x0f);
				nb[0] = '0' + ((byt >> 4) & 0x0f);
				if(nb[0] > '9') { nb[0] += 'A' - ('0' + 10); }
				if(nb[1] > '9') { nb[1] += 'A' - ('0' + 10); }
				s.append(nb,2);
			}
			r += s;
		}
	}
	s.clear();
	for(int i = 0; i < rbcount; i+= 8) {
		byt = (uint8_t)(rbits >> i);
		nb[1] = '0' + (byt & 0x0f);
		nb[0] = '0' + ((byt >> 4) & 0x0f);
		if(nb[0] > '9') { nb[0] += 'A' - ('0' + 10); }
		if(nb[1] > '9') { nb[1] += 'A' - ('0' + 10); }
		s.append(nb,2);
	}
	r += s;
	return r;
}

std::string BitString::GetHexBE()
{
	std::string r;
	std::string s;
	uint8_t byt;
	char nb[2];
	bitstore_t cx;
	s.clear();
	for(int i = 0; i < lbcount; i+= 8) {
		byt = (uint8_t)(lbits >> i);
		nb[1] = '0' + (byt & 0x0f);
		nb[0] = '0' + ((byt >> 4) & 0x0f);
		if(nb[0] > '9') { nb[0] += 'A' - ('0' + 10); }
		if(nb[1] > '9') { nb[1] += 'A' - ('0' + 10); }
		s.insert(0,nb,2);
	}
	r += s;
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
	for(int i = 0; i < rbcount; i+= 8) {
		byt = (uint8_t)(rbits >> i);
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
	if(rbcount + c <= 8*sizeof(bitstore_t)) {
		v &= 0xFFFFffff >> (32 - c);
		rbits = (rbits << c) | v; 
		rbcount += c;
	} else {
		if(rbcount == 32) {
			bits.push_back(rbits);
			v &= 0xFFFFffff >> (32 - c);
			rbcount = c; rbits = v;
			return;
		} else { // overrun
			int x = 32 - rbcount;
			bitstore_t t = (v >> (32-x)) & (0xFFFFffff >> (32 - x));
			rbits = (rbits << x) | t;
			c -= x;
			bits.push_back(rbits);
			v &= 0xFFFFffff >> (32 - c);
			rbcount = c; rbits = v;
		}
	}
}

void BitString::AddLeft(unsigned long v, int c)
{
	if(lbcount + c <= 8*sizeof(bitstore_t)) {
		v &= 0xFFFFffff >> (32 - c);
		lbits = (lbits) | (v << (lbcount)); 
		lbcount += c;
	} else {
		if(lbcount == 32) {
			bits.push_front(lbits);
			v &= 0xFFFFffff >> (32 - c);
			lbcount = c; lbits = v;
			return;
		} else { // overrun
			int x = 32 - lbcount;
			bitstore_t t = (v) & (0xFFFFffff >> (32 - x));
			lbits = (lbits) | (t << lbcount);
			c -= x;
			bits.push_front(lbits);
			v >> lbcount;
			v &= 0xFFFFffff >> (32 - x);
			lbcount = c; lbits = v;
		}
	}
}

