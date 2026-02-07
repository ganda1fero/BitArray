#ifndef BITARRAY_H
#define BITARRAY_H

#include <cstdint>
#include <exception>
#include <string>

#include <iostream>
#include <iomanip>

template<size_t Bits>
class BitArray {
private:
	const uint64_t mask_;
	uint64_t* memory_;

	size_t size_;
	size_t capacity_;

	inline const uint64_t get_mask() const;
	inline bool is_overflow(const uint64_t& val) const;

	class BitArrayRef {
	private:
		uint64_t* place_ptr;
		uint32_t bit_index;
		BitArray<Bits>* ref_ptr;

		inline BitArrayRef(BitArray<Bits>* ref_ptr, uint64_t* place_ptr, uint32_t bit_index);
		inline BitArrayRef(const BitArray<Bits>::BitArrayRef& other);
	public:
		inline operator uint64_t() const;

		inline BitArrayRef& operator=(const uint64_t& other);
		inline BitArrayRef& operator+=(const uint64_t& other);
		inline BitArrayRef& operator-=(const uint64_t& other);
		inline BitArrayRef& operator*=(const uint64_t& other);
		inline BitArrayRef& operator/=(const uint64_t& other);
		inline BitArrayRef& operator++();	// previx
		inline BitArrayRef& operator--();	// prefix
		inline uint64_t operator++(int);	// postfix
		inline uint64_t operator--(int);	// postfix
	};
public:
	inline BitArray();
	inline BitArray(const std::initializer_list<uint64_t>& init_list);
	inline ~BitArray();

	inline size_t size() const;
	inline size_t capacity() const;
	inline bool empty() const;

	inline BitArrayRef front();
	inline BitArrayRef back();

	inline typename BitArray<Bits>::iterator begin() const;
	inline typename BitArray<Bits>::iterator end() const;
	
	inline void resize(size_t new_size);
	inline void reserve(size_t new_capacity);
	inline void clear();

	inline void pop_back();
	inline void push_back(const uint64_t val);	//#

	void erase(size_t begin_index, size_t end_index);

	void insert(size_t index, const uint64_t val);

	inline BitArrayRef operator[](size_t index);
	inline BitArray& operator=(const BitArray& other);
	inline BitArray& operator=(std::initializer_list<uint64_t>& init_list);

	class iterator {
	private:
		BitArrayRef bit_ref;
		iterator(BitArray<Bits>* ref_ptr, uint64_t* place_ptr, uint32_t bit_index);
	public:
		iterator();
		iterator(const BitArray<Bits>::iterator& other_it);

		inline BitArrayRef& operator*();
		inline iterator& operator++();	// prefix
		inline iterator& operator--();	// prefix
		inline iterator& operator=(const iterator& other);
		inline bool operator==(const iterator& other);
		inline bool operator!=(const iterator& other);
	};
};

// implementation

// BitArray
template<size_t Bits>
inline const uint64_t BitArray<Bits>::get_mask() const {
	static_assert(Bits >= 1 && Bits <= 64, "Bits must be in [1..64]");
	uint64_t mask;
	if constexpr (Bits == 64) {	// if на этапе компиляции
		mask = ~uint64_t(0);
	}
	else {
		mask = (uint64_t(1) << Bits) - 1;
	}

	return mask;
}

template<size_t Bits>
inline bool BitArray<Bits>::is_overflow(const uint64_t& val) const {
	return val > mask_;
}

template<size_t Bits>
inline BitArray<Bits>::BitArray() : mask_(get_mask()) {
	memory_ = nullptr;
	size_ = 0;
	capacity_ = 0;
}

template<size_t Bits>
inline BitArray<Bits>::~BitArray() {
	clear();
}

template<size_t Bits>
inline size_t BitArray<Bits>::size() const {
	return size_;
}

template<size_t Bits>
inline size_t BitArray<Bits>::capacity() const {
	return capacity_;
}

template<size_t Bits>
inline bool BitArray<Bits>::empty() const {
	return !static_cast<bool>(size_);
}

template<size_t Bits>
inline typename BitArray<Bits>::BitArrayRef BitArray<Bits>::front() {
	if (empty()) {
		throw std::out_of_range("Out of range. BitArray is empty");
	}

	return BitArray<Bits>::BitArrayRef(this, &memory_[0], 0);
}

template<size_t Bits>
inline typename BitArray<Bits>::BitArrayRef BitArray<Bits>::back() {
	if (empty()) {
		throw std::out_of_range("Out of range. BitArray is empty");
	}

	return BitArray<Bits>::BitArrayRef(this, &memory_[(size_ - 1) * Bits / 64], ((size_ - 1) * Bits) % 64);
}

template<size_t Bits>
inline void BitArray<Bits>::resize(size_t new_size) {
	const size_t words_count = (size_ * Bits + 63) / 64;
	const size_t new_words_count = (new_size * Bits + 63) / 64;
	
	uint64_t* tmp_memory = new uint64_t[new_words_count];
	size_t tmp_i{};
	if (new_words_count <= words_count) {
		for (; tmp_i < new_words_count; ++tmp_i) {
			tmp_memory[tmp_i] = memory_[tmp_i];
		}
		if (new_size && (new_size * Bits) % 64 != 0) {
			tmp_memory[tmp_i - 1] &= ~((uint64_t(1) << (64 - (new_size * Bits) % 64)) - 1);
		}
	}
	else {	// >
		for (; tmp_i < words_count; ++tmp_i) {
			tmp_memory[tmp_i] = memory_[tmp_i];
		}
		for (; tmp_i < new_words_count; ++tmp_i) {
			tmp_memory[tmp_i] = 0;
		}
	}
	if (memory_ != nullptr) {
		delete[] memory_;
	}
	size_ = new_size;
	capacity_ = new_words_count * 64 / Bits;
	memory_ = tmp_memory;
}

template<size_t Bits>
void BitArray<Bits>::reserve(size_t new_capacity) {
	if (new_capacity <= capacity_) {
		return;
	}

    const size_t words_count = (size_ * Bits + 63) / 64;
    const size_t new_words_count = (new_capacity * Bits + 63) / 64;
    uint64_t* new_memory = new uint64_t[new_words_count];
    for (size_t i = 0; i < words_count; ++i) {
        new_memory[i] = memory_[i];
    }
    for (size_t i = words_count; i < new_words_count; ++i) {
        new_memory[i] = 0;
    }

    delete[] memory_;
    memory_ = new_memory;
    capacity_ = new_words_count * 64 / Bits;
}

template<size_t Bits>
inline void BitArray<Bits>::clear() {
	if (memory_ == nullptr) {
		delete[] memory_;
	}
	size_ = capacity_ = 0;
}

template<size_t Bits>
inline void BitArray<Bits>::pop_back() {
	if (empty()) {
		throw std::out_of_range("Out of range, BitArray is empty!");
	}

	const size_t next_bits{ (--size_) * Bits };

	if constexpr (64 % Bits == 0) {	// only in 1 word
		if (next_bits % 64 == 0) {	// stats in new word
			memory_[next_bits % 64] = 0;
		}
		else {
			memory_[next_bits % 64] &= ~((uint64_t(1) << (64 - next_bits % 64)) - 1);
		}
	}
	else {	// can be in 2 words
		if (next_bits % 64 + Bits > 64) {	// in 2 words
			memory_[next_bits % 64 + 1] = 0;
			memory_[next_bits % 64] &= ~((uint64_t(1) << (64 - next_bits % 64)) - 1);
		}
		else {	// in 1 word
			memory_[next_bits % 64] &= ~((uint64_t(1) << (64 - next_bits % 64)) - 1);
		}
	}
}

template<size_t Bits>
inline void BitArray<Bits>::push_back(const uint64_t val) {
	if (is_overflow(val)) {
		throw std::overflow_error("Overflow");
	}
	
	if (size_ == capacity_) {
		if (!size_) {
			reserve(1);
		}
		else {
			reserve(capacity_ * 2);
		}
	}

	const size_t bits_index = size_ * Bits;
	if constexpr (64 % Bits == 0) {	// only in 1 word
		memory_[bits_index / 64] |= val << (64 - bits_index % 64 - Bits);
	}
	else {	// can be in 2 words
		if (bits_index % 64 + Bits <= 64) {	// in 1 word
			memory_[bits_index / 64] |= val << (64 - bits_index % 64 - Bits);
		}
		else {	// in 2 words
			const int second_len = Bits - 64 + bits_index % 64;
			memory_[bits_index / 64 - 1] |= val >> (second_len);
			memory_[bits_index / 64] |= val << (64 - second_len);
		}
	}

	++size_;
}

template<size_t Bits>
inline typename BitArray<Bits>::BitArrayRef BitArray<Bits>::operator[](size_t index) {
	if (index >= size_) {
		throw std::out_of_range("Index " + std::to_string(index) + " out of range");
	}

	return BitArray<Bits>::BitArrayRef(this, &memory_[index * Bits / 64], (index * Bits) % 64);
}

template<size_t Bits>
inline BitArray<Bits>& BitArray<Bits>::operator=(const BitArray<Bits>& other) {
	if (&other == this) {
		return *this;
	}

	clear();
	size_ = other.size_;
	capacity_ = other.capacity_;
	memory_ = new uint64_t[capacity_];

	size_t word_count = (size_ * Bits + 63) / 64;
	uint64_t* this_ptr{ memory_ - 1 };
	uint64_t* other_ptr{ other.memory_ - 1 };
	for (size_t i{}; i < word_count; ++i) {
		*(++this_ptr) = *(++other_ptr);
	}

	return *this;
}

// BitArrayRef
template<size_t Bits>
inline BitArray<Bits>::BitArrayRef::BitArrayRef(BitArray<Bits>* ref_ptr, uint64_t* place_ptr, uint32_t bit_index) : ref_ptr(ref_ptr), place_ptr(place_ptr), bit_index(bit_index) {}

template<size_t Bits>
BitArray<Bits>::BitArrayRef::BitArrayRef(const BitArray<Bits>::BitArrayRef& other) : place_ptr(other.place_ptr), bit_index(other.bit_index), ref_ptr(other.ref_ptr) {}

template<size_t Bits>
inline BitArray<Bits>::BitArrayRef::operator uint64_t() const {
	uint64_t val;
	if constexpr (64 % Bits == 0) {	// elem only in 1 word
		val = *place_ptr >> (64 - bit_index - Bits);
	}
	else {	// elem can be in 2 words
		if (bit_index + Bits <= 64) {	// in 1 word
			val = *place_ptr >> (64 - bit_index - Bits);
		}
		else {	// in 2 words
			const int first_len = 64 - bit_index;
			const int second_len = Bits - first_len;
			val = 
				((*place_ptr & ((uint64_t(1) << first_len) - 1)) << second_len)
				| (*(place_ptr + 1) >> (64 - second_len));
		}
	}

	return val & ref_ptr->mask_;
}

template<size_t Bits>
inline typename BitArray<Bits>::BitArrayRef& BitArray<Bits>::BitArrayRef::operator=(const uint64_t& other) {
	if (other > ref_ptr->mask_) {
		throw std::overflow_error("Overflow");
	}

	if constexpr (64 % Bits == 0) {	// only in 1 word
		*place_ptr &= ~(((uint64_t(1) << Bits) - 1)
			<< (64 - bit_index - Bits));	// delete old value
		*place_ptr |= other << (64 - bit_index - Bits);	// set new value
	}
	else {	// can be in 2 words
		if (bit_index + Bits <= 64) {	// in 1 word
			*place_ptr &= ~(((uint64_t(1) << Bits) - 1)
				<< (64 - bit_index - Bits));	// delete old value
			*place_ptr |= other << (64 - bit_index - Bits);	// set new value
		}
		else {	// in 2 words
			const int first_len = 64 - bit_index;
			const int second_len = Bits - first_len;
			*place_ptr &= ~((uint64_t(1) << first_len) - 1);	// del first part
			*place_ptr |= other >> second_len;	// set first part value
			*(place_ptr + 1) &= ~(((uint64_t(1) << second_len) - 1) << (64 - second_len)); // del second value
			*(place_ptr + 1) |= other << (64 - second_len);
		}
	}

	return *this;
}

template<size_t Bits>
inline typename BitArray<Bits>::BitArrayRef& BitArray<Bits>::BitArrayRef::operator+=(const uint64_t& other) {
	*this = static_cast<uint64_t>(*this) + other;
	return *this;
}

template<size_t Bits>
inline typename BitArray<Bits>::BitArrayRef& BitArray<Bits>::BitArrayRef::operator-=(const uint64_t& other) {
	*this = static_cast<uint64_t>(*this) - other;
	return *this;
}

template<size_t Bits>
inline typename BitArray<Bits>::BitArrayRef& BitArray<Bits>::BitArrayRef::operator*=(const uint64_t& other) {
	*this = static_cast<uint64_t>(*this) * other;
	return *this;
}

template<size_t Bits>
inline typename BitArray<Bits>::BitArrayRef& BitArray<Bits>::BitArrayRef::operator/=(const uint64_t& other) {
	*this = static_cast<uint64_t>(*this) / other;
	return *this;
}

template<size_t Bits>
inline typename BitArray<Bits>::BitArrayRef& BitArray<Bits>::BitArrayRef::operator++() {
	*this = static_cast<uint64_t>(*this) + 1;
	return *this;
}

template<size_t Bits>
inline typename BitArray<Bits>::BitArrayRef& BitArray<Bits>::BitArrayRef::operator--() {
	*this = static_cast<uint64_t>(*this) - 1;
	return *this;
}

template<size_t Bits>
inline uint64_t BitArray<Bits>::BitArrayRef::operator++(int) {
	uint64_t val = static_cast<uint64_t>(*this);
	*this = static_cast<uint64_t>(*this) + 1;
	return val;
}

template<size_t Bits>
inline uint64_t BitArray<Bits>::BitArrayRef::operator--(int) {
	uint64_t val = static_cast<uint64_t>(*this);
	*this = static_cast<uint64_t>(*this) - 1;
	return val;
}

// iterator
template<size_t Bits>
BitArray<Bits>::iterator::iterator(BitArray<Bits>* ref_ptr, uint64_t* place_ptr, uint32_t bit_index) : bit_ref(BitArray<Bits>::BitArrayRef(ref_ptr, place_ptr, bit_index)) {}

template<size_t Bits>
BitArray<Bits>::iterator::iterator() : bit_ref(BitArray<Bits>::BitArrayRef(nullptr, nullptr, 0)) {}

template<size_t Bits>
BitArray<Bits>::iterator::iterator(const BitArray<Bits>::iterator& other_it) : bit_ref(other_it.bit_ref) {}

#endif