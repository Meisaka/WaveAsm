
#include <string>

typedef unsigned char encoder_t;

typedef struct {
	std::string op;
	int arc;
	std::string arf;
	std::string enc;
	encoder_t * encode;
} opcodedef;

typedef struct {
	std::string reg;
	std::string nam;
	std::string enc;
	encoder_t * encode;
} regdef;

typedef struct {
	std::string rlo;
	std::string rhi;
	std::string nam;
	std::string enc;
	encoder_t encode;
} litdef;

typedef struct {
	std::string label;
	std::string op;
	std::string arg;

	uintaddr_t lnum;
	uintaddr_t fnum;
	std::string ltxt;
} linedef;

