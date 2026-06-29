#pragma once

#include <string_view>
#include <span>

class MutableStateVectorView;

class IGate 
{
	public:
		virtual void apply([[maybe_unused]] MutableStateVectorView &mutableStateVectorView, [[maybe_unused]] std::span<const size_t> qubits) const = 0;
		virtual size_t qubits_count() const = 0;
		virtual std::string_view name() const = 0;
		virtual ~IGate() = default;
};
