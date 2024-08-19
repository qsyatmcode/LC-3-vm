#include "LC-3.h"

int main(int argc, const char* argv[])
{
	using namespace LC3;

	auto vm{ VM::Init() };

	vm->Run(argc, argv);

	return 0;
}