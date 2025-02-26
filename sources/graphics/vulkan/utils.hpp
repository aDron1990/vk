#pragma once

template <typename T>
constexpr size_t alignedSize(size_t aligment)
{
	auto align = alignof(T) > aligment ? alignof(T) : aligment;
	return (sizeof(T) + align - 1) & ~(align - 1);
}
