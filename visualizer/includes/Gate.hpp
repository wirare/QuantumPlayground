#pragma once

#include <IGate.hpp>
#include <StateVector.hpp>
#include <Matrix.hpp>
#include <cmath>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#define DEFINE_NON_MATRIX_GATE(gateName, qubits_number, ...)																			\
	class gateName : public IGate																										\
	{																																	\
		public:																															\
			void apply([[maybe_unused]] MutableStateVectorView &state, [[maybe_unused]] std::span<const size_t> qubits) const override	\
			{																															\
				__VA_ARGS__;																												\
			}																															\
																																		\
			size_t qubits_count() const override { return qubits_number; }																\
			std::string_view name() const override {return #gateName;}																	\
	}																																	\

namespace Gate
{
	constexpr Complex I{0.0, 1.0};
	
	DEFINE_NON_MATRIX_GATE(I, 1, { return; });

	DEFINE_NON_MATRIX_GATE(X, 1, 
			{
				const size_t mask = size_t{1} << qubits[0];

				for (size_t i = 0; i != state.amplitude_count(); i++)
				{
					if ((i & mask) != 0)
						continue;

					const size_t j = i | mask;

					std::swap(state.amplitude(i), state.amplitude(j));
				}
			});

	DEFINE_NON_MATRIX_GATE(Y, 1, 
			{
				const size_t mask = size_t{1} << qubits[0];

				for (size_t i = 0; i != state.amplitude_count(); i++)
				{
					if ((i & mask) != 0)
						continue;

					const size_t j = i | mask;

					const Complex old0 = state.amplitude(i);
					const Complex old1 = state.amplitude(j);

					state.amplitude(i) = -I * old1;
					state.amplitude(j) = I * old0;
				}
			});

	DEFINE_NON_MATRIX_GATE(Z, 1, 
			{
				const size_t mask = size_t{1} << qubits[0];

				for (size_t i = 0; i != state.amplitude_count(); i++)
				{
					if ((i & mask) == 0)
						continue;

					state.amplitude(i) = -state.amplitude(i);
				}
			 });

	DEFINE_NON_MATRIX_GATE(H, 1, 
			{
				const size_t mask = size_t{1} << qubits[0];
				const double inv_sqrt_2 = 1.0 / std::sqrt(2.0);

				for (size_t i = 0; i != state.amplitude_count(); i++)
				{
					if ((i & mask) != 0)
						continue;
					
					const size_t j = i | mask;

					const Complex old0 = state.amplitude(i);
					const Complex old1 = state.amplitude(j);

					state.amplitude(i) = (old0 + old1) * inv_sqrt_2;
					state.amplitude(j) = (old0 - old1) * inv_sqrt_2;
				}
			});

	DEFINE_NON_MATRIX_GATE(S, 1,
			{
				const size_t mask = size_t{1} << qubits[0];

				for (size_t i = 0; i != state.amplitude_count(); i++)
				{
					if ((i & mask) == 0)
						continue;

					state.amplitude(i) = I * state.amplitude(i);
				}
			});

	DEFINE_NON_MATRIX_GATE(Sdg, 1,
			{
				const size_t mask = size_t{1} << qubits[0];

				for (size_t i = 0; i != state.amplitude_count(); i++)
				{
					if ((i & mask) == 0)
						continue;

					state.amplitude(i) = -I * state.amplitude(i);
				}
			});
}
