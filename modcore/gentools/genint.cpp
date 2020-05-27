#include <iostream>
#include <set>

using namespace std;

//Configuration
#define num_ints 256
set<int> has_fault = {8,10,11,12,13,14,17};

int main() {
	// Generate interrupt handler basis
	cout << ".section .text" << endl;
	for (int i=0; i<num_ints; i++) {
		cout << ".global _isr_base_" << i << endl;
		cout << "_isr_base_" << i << ":" << endl;
		if (has_fault.count(i) == 0)
			cout << "\tpushl $12345678" << endl;
		cout << "\tpushl $" << i << endl;
		cout << "\tjmp _isr_common" << endl;
	}
	cout << endl;
}
