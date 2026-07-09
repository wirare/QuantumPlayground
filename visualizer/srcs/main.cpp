#include <ActorFactory.hpp>
#include <BlochStepVisualizer.hpp>
#include <QuantumCircuit.hpp>
#include <Algorithms/Grover.hpp>

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

int main(int ac, char **av)
{
	if (ac < 2)
		return 1;

	QuantumCircuit oracle = phase_oracle_builder(av[1]);
	QuantumCircuit qc = build_grover_algo_circuit(strlen(av[1]), oracle);

	auto visualizer = BlochStepVisualizer(strlen(av[1]), qc);
	visualizer.show();
}
