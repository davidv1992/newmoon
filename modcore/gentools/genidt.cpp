#include <iostream>
#include <set>

using namespace std;

//Configuration
#define num_ints 256

int main() {
	// Generate void idtbuildtable
	cout << "#include <stdint.h>" << endl;
	cout << endl;
	cout << "extern uint16_t idt[];" << endl;
	cout << endl;
	for (int i=0; i<num_ints; i++) {
		cout << "void _isr_base_" << i << "();" << endl;
	}
	cout << endl;
	cout << "void idtbuildtable() {" << endl;
	cout << "\tuint32_t cur_address;" << endl;
	for (int i=0; i<num_ints; i++) {
		cout << "\tcur_address = (uint32_t) &_isr_base_" << i << ";" << endl;
		cout << "\tidt[" << i*4 << "] = cur_address & 0xFFFF;" << endl;
		cout << "\tidt[" << i*4+1 << "] = 0x08;" << endl;
		cout << "\tidt[" << i*4+2 << "] = 0x8E00;" << endl;
		cout << "\tidt[" << i*4+3 << "] = (cur_address >> 16) & 0xFFFF;" << endl;
	}
	cout << "}" << endl;
}
