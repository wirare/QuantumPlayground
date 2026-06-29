#include <StateVector.hpp>
#include <Gate.hpp>

int main(int ac, char **av)
{
	(void)ac;
	(void)av;
	StateVector sv(5);
	
	sv.gate(Gate::I(), {0});

	return 1;
}
