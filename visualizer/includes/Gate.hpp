#pragma once

#include <IGate.hpp>
#include <GateDetail.hpp>
#include <StateVector.hpp>
#include <Matrix.hpp>
#include <cmath>
#include <span>
#include <string_view>

#define BASE_NON_MATRIX_GATE(gateName, gateNameSmall, qubits_number, angular_params_number, multi_qubit, ...)							\
	class gateNameSmall : public IGate																									\
	{																																	\
		public:																															\
			void apply([[maybe_unused]] MutableStateVectorView &state,																	\
					   [[maybe_unused]] std::span<const size_t> qubits,																	\
					   [[maybe_unused]] std::span<const double> angular_params) const override											\
			{																															\
				__VA_ARGS__;																											\
			}																															\
																																		\
			bool is_multi_qubit_gate() const override { return multi_qubit; }															\
			std::string_view name() const override { return #gateName; }																\
			size_t angular_params_count() const override { return angular_params_number; }												\
			size_t qubits_count() const override { return qubits_number; }																\
	};																																	\
	inline const gateNameSmall &gateName() { static const gateNameSmall gate; return gate; }											\

#define DEFINE_NON_MATRIX_MULTI_QUBITS_GATE(gateName, gateNameSmall, min_qubit, angular_params_number, ...) \
	BASE_NON_MATRIX_GATE(gateName, gateNameSmall, min_qubit, angular_params_number, true, __VA_ARGS__)


#define DEFINE_NON_MATRIX_GATE(gateName, gateNameSmall, qubits_number, angular_params_number, ...) \
	BASE_NON_MATRIX_GATE(gateName, gateNameSmall, qubits_number, angular_params_number, false, __VA_ARGS__)

#define DEFINE_MATRIX_GATE(gateName, gateNameSmall, qubits_number, angular_params_number, ...)											\
    class gateNameSmall : public IGate																									\
    {																																	\
		public:																															\
			void apply([[maybe_unused]] MutableStateVectorView& state,																	\
					   [[maybe_unused]] std::span<const size_t> qubits,																	\
					   [[maybe_unused]] std::span<const double> angular_params) const override											\
			{																															\
				const Matrix<Complex> gate_matrix = matrix(angular_params);																\
				Gate::detail::apply_matrix_gate(state, qubits, gate_matrix);															\
			}																															\
																																		\
			size_t qubits_count() const override { return qubits_number; }																\
			std::string_view name() const override { return #gateName; }																\
			size_t angular_params_count() const override { return angular_params_number; }												\
			bool is_multi_qubit_gate() const override { return false; }																	\
			Matrix<Complex> matrix([[maybe_unused]] std::span<const double> angular_params) const override								\
			{																															\
				__VA_ARGS__																												\
			}																															\
    };																																	\
	inline const gateNameSmall &gateName() { static const gateNameSmall gate; return gate; }											\

namespace Gate
{
	constexpr Complex Im{0.0, 1.0};
	constexpr Complex Zero{0.0, 0.0};
	constexpr Complex One{1.0, 0.0};
	
	DEFINE_NON_MATRIX_GATE(I, i, 1, 0, { return; });

	DEFINE_NON_MATRIX_GATE(X, x, 1, 0,
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

	DEFINE_NON_MATRIX_GATE(Y, y, 1, 0,
	{
		const size_t mask = size_t{1} << qubits[0];

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & mask) != 0)
				continue;

			const size_t j = i | mask;

			const Complex old0 = state.amplitude(i);
			const Complex old1 = state.amplitude(j);

			state.amplitude(i) = -Im * old1;
			state.amplitude(j) = Im * old0;
		}
	});

	DEFINE_NON_MATRIX_GATE(Z, z, 1, 0,
	{
		const size_t mask = size_t{1} << qubits[0];

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & mask) == 0)
				continue;

			state.amplitude(i) = -state.amplitude(i);
		}
	 });

	DEFINE_NON_MATRIX_GATE(H, h, 1, 0,
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

	DEFINE_NON_MATRIX_GATE(S, s, 1, 0,
	{
		const size_t mask = size_t{1} << qubits[0];

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & mask) == 0)
				continue;

			state.amplitude(i) = Im * state.amplitude(i);
		}
	});

	DEFINE_NON_MATRIX_GATE(Sdg, sdg, 1, 0,
	{
		const size_t mask = size_t{1} << qubits[0];

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & mask) == 0)
				continue;

			state.amplitude(i) = -Im * state.amplitude(i);
		}
	});

	DEFINE_NON_MATRIX_GATE(T, t, 1, 0,
	{
		const size_t mask = size_t{1} << qubits[0];

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & mask) == 0)
				continue;

			state.amplitude(i) = std::exp(Im * (std::numbers::pi / 4)) * state.amplitude(i);
		}
	});

	DEFINE_NON_MATRIX_GATE(Tdg, tdg, 1, 0,
	{
		const size_t mask = size_t{1} << qubits[0];

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & mask) == 0)
				continue;

			state.amplitude(i) = std::exp(-Im * (std::numbers::pi / 4)) * state.amplitude(i);
		}
	});

	DEFINE_NON_MATRIX_GATE(P, p, 1, 1,
	{
		const size_t mask = size_t{1} << qubits[0];

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & mask) == 0)
				continue;

			state.amplitude(i) = std::exp(Im * angular_params[0]) * state.amplitude(i);
		}
	});

	DEFINE_NON_MATRIX_GATE(RZ, rz, 1, 1,
	{
		const size_t mask = size_t{1} << qubits[0];

		const double half_theta = angular_params[0] / 2;

		const Complex phase0 = std::exp(-Im * half_theta);
		const Complex phase1 = std::exp(Im * half_theta);

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & mask) == 0)
				state.amplitude(i) *= phase0;
			else
				state.amplitude(i) *= phase1;
		}
	});

	DEFINE_NON_MATRIX_GATE(RX, rx, 1, 1,
	{
		const size_t mask = size_t{1} << qubits[0];

		const double cos_half_theta = std::cos(angular_params[0] / 2);
		const double sin_half_theta = std::sin(angular_params[0] / 2);

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & mask) != 0)
				continue;

			const size_t j = i | mask;

			const Complex old0 = state.amplitude(i);
			const Complex old1 = state.amplitude(j);

			state.amplitude(i) = (cos_half_theta * old0) - (Im * sin_half_theta * old1);
			state.amplitude(j) = (-Im * sin_half_theta * old0) + (cos_half_theta * old1);
		}
	});

	DEFINE_NON_MATRIX_GATE(RY, ry, 1, 1,
	{
		const size_t mask = size_t{1} << qubits[0];

		const double cos_half_theta = std::cos(angular_params[0] / 2);
		const double sin_half_theta = std::sin(angular_params[0] / 2);

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & mask) != 0)
				continue;

			const size_t j = i | mask;

			const Complex old0 = state.amplitude(i);
			const Complex old1 = state.amplitude(j);

			state.amplitude(i) = (cos_half_theta * old0) + (sin_half_theta * old1);
			state.amplitude(j) = (-sin_half_theta * old0) + (cos_half_theta * old1);
		}
	});

	DEFINE_NON_MATRIX_GATE(CX, cx, 2, 0,
	{
		const size_t control = qubits[0];
		const size_t target = qubits[1];

		const size_t control_mask = size_t{1} << control;
		const size_t target_mask = size_t{1} << target;

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & control_mask) == 0)
				continue;

			if ((i & target_mask) != 0)
				continue;

			const size_t j = i | target_mask;

			std::swap(state.amplitude(i), state.amplitude(j));
		}
	});

	DEFINE_NON_MATRIX_GATE(CZ, cz, 2, 0,
	{
		const size_t control = qubits[0];
		const size_t target = qubits[1];

		const size_t control_mask = size_t{1} << control;
		const size_t target_mask = size_t{1} << target;

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & control_mask) == 0)
				continue;

			if ((i & target_mask) == 0)
				continue;

			state.amplitude(i) = -state.amplitude(i);
		}
	});

	DEFINE_NON_MATRIX_GATE(CP, cp, 2, 1,
	{
		const size_t control = qubits[0];
		const size_t target = qubits[1];

		const size_t control_mask = size_t{1} << control;
		const size_t target_mask = size_t{1} << target;

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & control_mask) == 0)
				continue;

			if ((i & target_mask) == 0)
				continue;

			state.amplitude(i) = std::exp(Im * angular_params[0]) * state.amplitude(i);
		}
	});
	
	DEFINE_NON_MATRIX_GATE(CRZ, crz, 2, 1,
	{
		const size_t control = qubits[0];
		const size_t target  = qubits[1];

		const size_t control_mask = size_t{1} << control;
		const size_t target_mask  = size_t{1} << target;

		const double half_theta = angular_params[0] / 2.0;

		const Complex phase0 = std::exp(-Im * half_theta);
		const Complex phase1 = std::exp(Im * half_theta);

		for (size_t i = 0; i != state.amplitude_count(); ++i)
		{
			if ((i & control_mask) == 0)
				continue;

			if ((i & target_mask) == 0)
				state.amplitude(i) *= phase0;
			else
				state.amplitude(i) *= phase1;
		}
	});

	DEFINE_NON_MATRIX_GATE(CRX, crx, 2, 1,
	{
		const size_t control = qubits[0];
		const size_t target  = qubits[1];

		const size_t control_mask = size_t{1} << control;
		const size_t target_mask  = size_t{1} << target;

		const double cos_half_theta = std::cos(angular_params[0] / 2);
		const double sin_half_theta = std::sin(angular_params[0] / 2);

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & control_mask) == 0)
				continue;

			if ((i & target_mask) != 0)
				continue;

			const size_t j = i | target_mask;

			const Complex old0 = state.amplitude(i);
			const Complex old1 = state.amplitude(j);

			state.amplitude(i) = (cos_half_theta * old0) - (Im * sin_half_theta * old1);
			state.amplitude(j) = (-Im * sin_half_theta * old0) + (cos_half_theta * old1);
		}
	});

	DEFINE_NON_MATRIX_GATE(CRY, cry, 2, 1,
	{
		const size_t control = qubits[0];
		const size_t target  = qubits[1];

		const size_t control_mask = size_t{1} << control;
		const size_t target_mask  = size_t{1} << target;

		const double cos_half_theta = std::cos(angular_params[0] / 2);
		const double sin_half_theta = std::sin(angular_params[0] / 2);

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & control_mask) == 0)
				continue;

			if ((i & target_mask) == 0)
				continue;

			const size_t j = i | target_mask;

			const Complex old0 = state.amplitude(i);
			const Complex old1 = state.amplitude(j);

			state.amplitude(i) = (cos_half_theta * old0) + (sin_half_theta * old1);
			state.amplitude(j) = (-sin_half_theta * old0) + (cos_half_theta * old1);
		}
	});

	DEFINE_NON_MATRIX_GATE(CNOT, cnot, 2, 0,
	{
		const size_t control = qubits[0];
		const size_t target	 = qubits[1];

		const size_t control_mask = size_t{1} << control;
		const size_t target_mask = size_t{1} << target;

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & control_mask) == 0)
				continue;

			if ((i & target_mask) != 0)
				continue;

			const size_t j = i | target_mask;

			std::swap(state.amplitude(i), state.amplitude(j));
		}
	});

	DEFINE_NON_MATRIX_GATE(SWAP, swap, 2, 0,
	{
		const size_t qA = qubits[0];
		const size_t qB = qubits[1];

		const size_t maskA = size_t{1} << qA;
		const size_t maskB = size_t{1} << qB;

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			const bool bitA = (i & maskA) != 0;
			const bool bitB = (i & maskB) != 0;

			if (bitA || !bitB)
				continue;

			const size_t j = i ^ maskA ^ maskB;

			std::swap(state.amplitude(i), state.amplitude(j));
		}
	});

	DEFINE_NON_MATRIX_GATE(ISWAP, iswap, 2, 0,
	{
		const size_t qA = qubits[0];
		const size_t qB = qubits[1];

		const size_t maskA = size_t{1} << qA;
		const size_t maskB = size_t{1} << qB;

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			const bool bitA = (i & maskA) != 0;
			const bool bitB = (i & maskB) != 0;

			if (bitA || !bitB)
				continue;

			const size_t j = i ^ maskA ^ maskB;

			const Complex oldA = state.amplitude(i);
			const Complex oldB = state.amplitude(j);

			state.amplitude(i) = oldB * Im;
			state.amplitude(j) = oldA * Im;
		}
	});

	DEFINE_NON_MATRIX_GATE(RZZ, rzz, 2, 1,
	{
		const size_t qA = qubits[0];
		const size_t qB = qubits[1];

		const size_t maskA = size_t{1} << qA;
		const size_t maskB = size_t{1} << qB;

		const double half_theta = angular_params[0] / 2;

		const Complex phase0 = std::exp(-Im * half_theta);
		const Complex phase1 = std::exp(Im * half_theta);

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			const bool bitA = (i & maskA) != 0;
			const bool bitB = (i & maskB) != 0;

			if (bitA == bitB)
				state.amplitude(i) *= phase0;
			else
				state.amplitude(i) *= phase1;
		}
	});

	DEFINE_NON_MATRIX_GATE(CCX, ccx, 3, 0,
	{
		const size_t control0 = qubits[0];
		const size_t control1 = qubits[1];
		const size_t target = qubits[2];

		const size_t control_mask = (size_t{1} << control0) | (size_t{1} << control1);	
		const size_t target_mask = size_t{1} << target;

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & control_mask) != control_mask)
				continue;

			if ((i & target_mask) != 0)
				continue;

			const size_t j = i | target_mask;

			std::swap(state.amplitude(i), state.amplitude(j));
		}
	});

	DEFINE_NON_MATRIX_GATE(CCZ, ccz, 3, 0,
	{
		const size_t control0 = qubits[0];
		const size_t control1 = qubits[1];
		const size_t control2 = qubits[2];

		const size_t control_mask = (size_t{1} << control0) | (size_t{1} << control1) | (size_t{1} << control2);

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & control_mask) != control_mask)
				continue;

			state.amplitude(i) *= -1;
		}
	});

	DEFINE_NON_MATRIX_GATE(CSWAP, cswap, 3, 0,
	{
		const size_t control = qubits[0];
		const size_t qA = qubits[1];
		const size_t qB = qubits[2];

		const size_t control_mask = size_t{1} << control;
		const size_t maskA = size_t{1} << qA;
		const size_t maskB = size_t{1} << qB;

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & control_mask) == 0)
				continue;

			const bool bitA = (i & maskA) != 0;
			const bool bitB = (i & maskB) != 0;

			if (bitA || !bitB)
				continue;

			const size_t j = i ^ maskA ^ maskB;

			std::swap(state.amplitude(i), state.amplitude(j));
		}
	});

	DEFINE_NON_MATRIX_MULTI_QUBITS_GATE(MCX, mcx, 2, 0,
	{
		const size_t target = qubits[qubits.size() - 1];

		size_t control_mask = 0;
		for (size_t i = 0; i != qubits.size() - 1; i++)
			control_mask |= size_t{1} << qubits[i];

		const size_t target_mask = size_t{1} << target;

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & control_mask) != control_mask)
				continue;

			if ((i & target_mask) != 0)
				continue;

			const size_t j = i | target_mask;

			std::swap(state.amplitude(i), state.amplitude(j));
		}
	});

	DEFINE_NON_MATRIX_MULTI_QUBITS_GATE(MCZ, mcz, 1, 0,
	{
		size_t control_mask = 0;
		for (size_t i = 0; i != qubits.size(); i++)
			control_mask |= size_t{1} << qubits[i];

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & control_mask) != control_mask)
				continue;

			state.amplitude(i) *= -1;
		}
	});

	DEFINE_NON_MATRIX_MULTI_QUBITS_GATE(MCP, mcp, 1, 1,
	{
		size_t control_mask = 0;
		for (size_t i = 0; i != qubits.size(); i++)
			control_mask |= size_t{1} << qubits[i];

		const Complex phase = std::exp(Im * angular_params[0]);

		for (size_t i = 0; i != state.amplitude_count(); i++)
		{
			if ((i & control_mask) != control_mask)
				continue;

			state.amplitude(i) *= phase;
		}
	});

	DEFINE_MATRIX_GATE(RXX, rxx, 2, 1,
	{
		const double half_theta = angular_params[0] / 2.0;
		const double cos = std::cos(half_theta);
		const double sin = std::sin(half_theta);

		return Matrix<Complex>(4, 4,
			{
				Complex{cos, 0.0},	Zero,				Zero,				-Im * sin,
				Zero,				Complex{cos, 0.0},	-Im * sin,			Zero,
				Zero,				-Im * sin,			Complex{cos, 0.0},	Zero,
				-Im * sin,			Zero,				Zero,				Complex{cos, 0.0},
			});
	});

	DEFINE_MATRIX_GATE(RYY, ryy, 2, 1,
	{
		const double half_theta = angular_params[0] / 2.0;
		const double cos = std::cos(half_theta);
		const double sin = std::sin(half_theta);

		return Matrix<Complex>(4, 4,
			{
				Complex{cos, 0.0},	Zero,				Zero,				Im * sin,
				Zero,				Complex{cos, 0.0},	-Im * sin,			Zero,
				Zero,				-Im * sin,			Complex{cos, 0.0},	Zero,
				Im * sin,			Zero,				Zero,				Complex{cos, 0.0}
			}
		);
	});

	DEFINE_MATRIX_GATE(RZX, rzx, 2, 1,
	{
		const double half_theta = angular_params[0] / 2.0;
		const double cos = std::cos(half_theta);
		const double sin = std::sin(half_theta);

		return Matrix<Complex>(4, 4,
			{
				Complex{cos, 0.0},	Zero,				-Im * sin,			Zero,
				Zero,				Complex{cos,0.0},	Zero,				Im * sin,
				-Im * sin,			Zero,				Complex{cos,0.0},	Zero,
				Zero,				Im * sin,			Zero,				Complex{cos,0.0}
			}
		);
	});

	DEFINE_MATRIX_GATE(ECR, ecr, 2, 0,
	{
		const double r = 1.0 / std::sqrt(2.0);

		return Matrix<Complex>(4, 4,
			{
				Zero,				Zero,				Complex{r, 0.0},	Im * r,
				Zero,				Zero,				Im * r,				Complex{r, 0.0},
				Complex{r, 0.0},	-Im * r,			Zero,				Zero,
				-Im * r,			Complex{r, 0.0},	Zero,				Zero
			}
		);
	});

	DEFINE_MATRIX_GATE(U, u, 1, 3,
	{
		const double theta  = angular_params[0];
		const double phi    = angular_params[1];
		const double lambda = angular_params[2];

		const double cos = std::cos(theta / 2.0);
		const double sin = std::sin(theta / 2.0);

		return Matrix<Complex>(2, 2,
			{
				Complex{cos, 0.0},          -std::exp(Im * lambda) * sin,
				std::exp(Im * phi) * sin,	std::exp(Im * (phi + lambda)) * cos
			}
		);
	});

	DEFINE_MATRIX_GATE(SX, sx, 1, 0,
	{
		return Matrix<Complex>(2, 2,
			{
				Complex{0.5,  0.5}, Complex{0.5, -0.5},
				Complex{0.5, -0.5}, Complex{0.5,  0.5}
			}
		);
	});

	DEFINE_MATRIX_GATE(CH, ch, 2, 0,
	{
		const double r = 1.0 / std::sqrt(2.0);

		return Matrix<Complex>(4, 4,
			{
				One,  Zero,              Zero, Zero,
				Zero, Complex{r, 0.0},   Zero, Complex{r, 0.0},
				Zero, Zero,              One,  Zero,
				Zero, Complex{r, 0.0},   Zero, Complex{-r, 0.0}
			}
		);
	});

	DEFINE_MATRIX_GATE(SQRTSWAP, sqrtswap, 2, 0,
	{
		const Complex a{0.5,  0.5};
		const Complex b{0.5, -0.5};

		return Matrix<Complex>(
			4,
			4,
			{
				One,  Zero, Zero, Zero,
				Zero, a,    b,    Zero,
				Zero, b,    a,    Zero,
				Zero, Zero, Zero, One
			}
		);
	});
}

