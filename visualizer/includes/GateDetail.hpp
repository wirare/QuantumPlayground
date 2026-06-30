#pragma once

#include <vector>
#include <Matrix.hpp>
#include <span>
#include <complex>
#include <StateVector.hpp>

namespace Gate::detail
{
	using Complex = std::complex<double>;

    struct LocalIndexMap
    {
        size_t selected_mask;
        std::vector<size_t> local_to_full_mask;
    };

    inline LocalIndexMap make_local_index_map(std::span<const size_t> qubits)
    {
        const size_t k = qubits.size();
        const size_t local_dim = size_t{1} << k;

        LocalIndexMap map;
        map.selected_mask = 0;
        map.local_to_full_mask.resize(local_dim);

        std::vector<size_t> qubit_masks(k);

        for (size_t local_bit = 0; local_bit != k; ++local_bit)
        {
            qubit_masks[local_bit] = size_t{1} << qubits[local_bit];
            map.selected_mask |= qubit_masks[local_bit];
        }

        for (size_t local_index = 0; local_index != local_dim; ++local_index)
        {
            size_t full_mask = 0;

            for (size_t local_bit = 0; local_bit != k; ++local_bit)
            {
                if ((local_index & (size_t{1} << local_bit)) != 0)
                    full_mask |= qubit_masks[local_bit];
            }

            map.local_to_full_mask[local_index] = full_mask;
        }

        return map;
    }

    inline Matrix<Complex> gather_local_vector(
        MutableStateVectorView& state,
        size_t base_index,
        const std::vector<size_t>& local_to_full_mask
    )
    {
        const size_t local_dim = local_to_full_mask.size();

        Matrix<Complex> input(local_dim, 1);

        for (size_t local_index = 0; local_index != local_dim; ++local_index)
        {
            const size_t full_index = base_index | local_to_full_mask[local_index];
            input(local_index, 0) = state.amplitude(full_index);
        }

        return input;
    }

    inline void scatter_local_vector(
        MutableStateVectorView& state,
        size_t base_index,
        const std::vector<size_t>& local_to_full_mask,
        const Matrix<Complex>& output
    )
    {
        const size_t local_dim = local_to_full_mask.size();

        for (size_t local_index = 0; local_index != local_dim; ++local_index)
        {
            const size_t full_index = base_index | local_to_full_mask[local_index];
            state.amplitude(full_index) = output(local_index, 0);
        }
    }

    inline void apply_matrix_gate(
        MutableStateVectorView& state,
        std::span<const size_t> qubits,
        const Matrix<Complex>& gate_matrix
    )
    {
        const LocalIndexMap map = make_local_index_map(qubits);

        for (size_t base_index = 0; base_index != state.amplitude_count(); base_index++)
        {
            if ((base_index & map.selected_mask) != 0)
                continue;

            Matrix<Complex> input = gather_local_vector(state, base_index, map.local_to_full_mask);
            Matrix<Complex> output = gate_matrix.mul_mat(input);
            scatter_local_vector(state, base_index, map.local_to_full_mask, output);
        }
    }
}
