#ifndef BITARRAY_H
#define BITARRAY_H

#include <cstdint>
#include <exception>
#include <string>
#include <assert.h>
#include <type_traits>

template<size_t Bits>
class BitArray {
private:
	const uint64_t mask_ = get_mask();
	uint64_t* memory_;

	size_t size_;
	size_t capacity_;

	inline const uint64_t get_mask() const;
	inline bool is_overflow(const uint64_t& val) const;

	template<typename T_it>
	void init_from_range(const T_it& beg_it, const T_it& end_it);
	template<typename T_it>
	void add_from_range(const T_it& beg_it, const T_it& end_it);

	class BitArrayRef {
	private:
		uint64_t* place_ptr;
		uint32_t bit_index;
		BitArray<Bits>* ref_ptr;

		inline BitArrayRef(BitArray<Bits>* ref_ptr, uint64_t* place_ptr, uint32_t bit_index);
		inline BitArrayRef(const BitArray<Bits>::BitArrayRef& other);
		friend class BitArray<Bits>;
		friend class BitArray<Bits>::iterator;
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
		inline bool operator==(const BitArray<Bits>::BitArrayRef& other_ref) const;
		inline bool operator!=(const BitArray<Bits>::BitArrayRef& other_ref) const;
	};
public:
	class iterator {
	private:
		BitArrayRef bit_ref;
		inline iterator(BitArray<Bits>* ref_ptr, uint64_t* place_ptr, uint32_t bit_index);
		friend class BitArray<Bits>;
	public:
		iterator();
		iterator(const BitArray<Bits>::iterator& other_it);

		inline BitArrayRef& operator*();
		inline iterator& operator++();	// prefix
		inline iterator& operator--();	// prefix
		inline iterator& operator+=(const uint64_t& val);
		inline iterator& operator-=(const uint64_t& val);
		inline iterator operator+(size_t value) const;
		inline iterator operator-(size_t value) const;
		inline iterator& operator=(const BitArray<Bits>::iterator& other);
		inline bool operator==(const BitArray<Bits>::iterator& other) const;
		inline bool operator!=(const BitArray<Bits>::iterator& other) const;
		inline bool operator<(const BitArray<Bits>::iterator& other) const;
		inline bool operator>(const BitArray<Bits>::iterator& other) const;
		inline bool operator<=(const BitArray<Bits>::iterator& other) const;
		inline bool operator>=(const BitArray<Bits>::iterator& other) const;
	};

	inline BitArray();
	~BitArray();
	template<typename T> BitArray(const std::initializer_list<T>& init_list);
	template<typename T> BitArray(const std::vector<T>& vect);

	inline size_t size() const;
	inline size_t capacity() const;
	inline bool empty() const;

	inline BitArray<Bits>::BitArrayRef front();
	inline BitArray<Bits>::BitArrayRef back();

	inline BitArray<Bits>::iterator begin();
	inline BitArray<Bits>::iterator end();
	
	void resize(size_t new_size);
	void reserve(size_t new_capacity);
	void clear();

	inline void pop_back();
	void push_back(const uint64_t val);

	void erase(BitArray<Bits>::iterator beg_it, BitArray<Bits>::iterator end_it);

	void insert(BitArray<Bits>::iterator it, const uint64_t& val);
	void insert(BitArray<Bits>::iterator it, const uint64_t& val, const size_t count);

	inline BitArrayRef operator[](size_t index);
	BitArray& operator=(const BitArray<Bits>& other);
	template<typename T> BitArray& operator=(const std::initializer_list<T>& init_list);
	template<typename T> BitArray& operator=(const std::vector<T>& vect);
	template<typename T> BitArray& operator+=(const std::initializer_list<T>& init_list);
	template<typename T> BitArray& operator+=(const std::vector<T>& vect);
};

// implementation

// BitArray
template<size_t Bits>
inline const uint64_t BitArray<Bits>::get_mask() const {
	static_assert(Bits >= 1 && Bits <= 63, "Bits must be in [1..63]");
	uint64_t mask;
	if constexpr (Bits == 64) {	// if на этапе компиляции
		throw std::exception("Bits must be in [1...63]");
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
template<typename T_it>
void BitArray<Bits>::init_from_range(const T_it& beg_it, const T_it& end_it) {
	const size_t size = end_it - beg_it;

	// init memory
	const size_t word_count = (size * Bits + 63) / 64;
	memory_ = new uint64_t[word_count]{};	// init with nulls
	size_ = size;
	capacity_ = word_count * 64 / Bits;

	// fill via values
	auto this_it = this->begin();
	auto init_it = beg_it;
	while (init_it != end_it) {
		const uint64_t val = static_cast<const uint64_t>(*init_it);
		if (val > mask_) {
			throw std::overflow_error("Overflow");
		}
		if constexpr (64 % Bits == 0) {	// can be only in 1 word
			*((*this_it).place_ptr) |= val << (64 - (*this_it).bit_index - Bits);
		}
		else {	// can be in 2 words
			if ((*this_it).bit_index + Bits <= 64) {	// in 1 word
				*((*this_it).place_ptr) |= val << (64 - (*this_it).bit_index - Bits);
			}
			else {	// in 2 words
				const int second_len = Bits - 64 + (*this_it).bit_index;
				*((*this_it).place_ptr) |= val >> (second_len);	// put first part 
				*((*this_it).place_ptr + 1) |= val << (64 - second_len);	// put second part
			}
		}

		++this_it;
		++init_it;
	}
}

template<size_t Bits>
template<typename T_it>
void BitArray<Bits>::add_from_range(const T_it& beg_it, const T_it& end_it) {
	if constexpr (std::is_same_v<decltype(beg_it), decltype(this->begin())>) {
		if (beg_it == this->begin()) {
			throw std::exception("self copy banned!");
		}
	}
	
	const size_t size = end_it - beg_it;
	
	if (capacity_ < size_ + size) {	// needs to add capacity
		reserve(size_ + size);
	}

	auto this_it = this->end();
	auto add_it = beg_it;
	size_ += size;
	while (add_it != end_it) {
		const uint64_t val = static_cast<const uint64_t>(*add_it);
		if (val > mask_) {
			throw std::overflow_error("Overflow");
		}
		if constexpr (64 % Bits == 0) {	// can be only in 1 word
			*((*this_it).place_ptr) |= val << (64 - (*this_it).bit_index - Bits);
		}
		else {	// can be in 2 words
			if ((*this_it).bit_index + Bits <= 64) {	// in 1 word
				*((*this_it).place_ptr) |= val << (64 - (*this_it).bit_index - Bits);
			}
			else {	// in 2 words
				const int second_len = Bits - 64 + (*this_it).bit_index;
				*((*this_it).place_ptr) |= val >> (second_len);	// put first part 
				*((*this_it).place_ptr + 1) |= val << (64 - second_len);	// put second part
			}
		}

		++this_it;
		++add_it;
	}
}

template<size_t Bits>
inline BitArray<Bits>::BitArray() {
	memory_ = nullptr;
	size_ = 0;
	capacity_ = 0;
}

template<size_t Bits>
template<typename T>
BitArray<Bits>::BitArray(const std::initializer_list<T>& init_list) : BitArray() {
	init_from_range(init_list.begin(), init_list.end());
}

template<size_t Bits>
template<typename T>
BitArray<Bits>::BitArray(const std::vector<T>& vect) : BitArray() {
	init_from_range(vect.begin(), vect.end());
}

template<size_t Bits>
BitArray<Bits>::~BitArray() {
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
inline typename BitArray<Bits>::iterator BitArray<Bits>::begin() {
	BitArray<Bits>::iterator it(this, nullptr, 0);	// like empty
	
	if (size_) {	// not empty
		it.bit_ref.place_ptr = memory_;
		// bit_index still 0
	}

	return it;
}

template<size_t Bits>
inline typename BitArray<Bits>::iterator BitArray<Bits>::end() {
	BitArray<Bits>::iterator it(this, nullptr, 0);	// like_empty

	if (size_) {	// not empty
		it.bit_ref.place_ptr = memory_ + (size_ * Bits / 64);
		it.bit_ref.bit_index = (size_ * Bits) % 64;	// +1 (end)
	}

	return it;
}

template<size_t Bits>
void BitArray<Bits>::resize(size_t new_size) {
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
void BitArray<Bits>::clear() {
	if (memory_ != nullptr) {
		delete[] memory_;
	}
	size_ = capacity_ = 0;
	memory_ = nullptr;
}

template<size_t Bits>
inline void BitArray<Bits>::pop_back() {
	if (empty()) {
		throw std::out_of_range("Out of range, BitArray is empty!");
	}

	const size_t next_bits{ (--size_) * Bits };

	if constexpr (64 % Bits == 0) {	// only in 1 word
		if (next_bits % 64 == 0) {	// stats in new word
			memory_[next_bits / 64] = 0;
		}
		else {
			memory_[next_bits / 64] &= ~((uint64_t(1) << (64 - next_bits % 64)) - 1);
		}
	}
	else {	// can be in 2 words
		if (next_bits % 64 + Bits > 64) {	// in 2 words
			memory_[next_bits / 64 + 1] = 0;
			memory_[next_bits / 64] &= ~((uint64_t(1) << (64 - next_bits % 64)) - 1);
		}
		else {	// in 1 word
			memory_[next_bits / 64] &= ~((uint64_t(1) << (64 - next_bits % 64)) - 1);
		}
	}
}

template<size_t Bits>
void BitArray<Bits>::push_back(const uint64_t val) {
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
			memory_[bits_index / 64] |= val >> (second_len);
			memory_[bits_index / 64 + 1] |= val << (64 - second_len);
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
BitArray<Bits>& BitArray<Bits>::operator=(const BitArray<Bits>& other) {
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

template<size_t Bits>
template<typename T>
BitArray<Bits>& BitArray<Bits>::operator=(const std::initializer_list<T>& init_list) {
	clear();
	init_from_range(init_list.begin(), init_list.end());
	
	return *this;
}

template<size_t Bits>
template<typename T>
BitArray<Bits>& BitArray<Bits>::operator=(const std::vector<T>& vect) {
	clear();
	init_from_range(vect.begin(), vect.end());

	return *this;
}

template<size_t Bits>
template<typename T>
BitArray<Bits>& BitArray<Bits>::operator+=(const std::initializer_list<T>& init_list) {
	add_from_range(init_list.begin(), init_list.end());
	
	return *this;
}

template<size_t Bits>
template<typename T>
BitArray<Bits>& BitArray<Bits>::operator+=(const std::vector<T>& vect) {
	add_from_range(vect.begin(), vect.end());

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

template<size_t Bits>
inline bool BitArray<Bits>::BitArrayRef::operator==(const BitArray<Bits>::BitArrayRef& other_ref) const {
	return ref_ptr == other_ref.ref_ptr
		&& place_ptr == other_ref.place_ptr
		&& bit_index == other_ref.bit_index;
}

template<size_t Bits>
inline bool BitArray<Bits>::BitArrayRef::operator!=(const BitArray<Bits>::BitArrayRef& other_ref) const {
	return !(*this == other_ref);
}

// iterator
template<size_t Bits>
inline BitArray<Bits>::iterator::iterator(BitArray<Bits>* ref_ptr, uint64_t* place_ptr, uint32_t bit_index) : bit_ref(BitArray<Bits>::BitArrayRef(ref_ptr, place_ptr, bit_index)) {}

template<size_t Bits>
inline BitArray<Bits>::iterator::iterator() : bit_ref(BitArray<Bits>::BitArrayRef(nullptr, nullptr, 0)) {}

template<size_t Bits>
inline BitArray<Bits>::iterator::iterator(const BitArray<Bits>::iterator& other_it) : bit_ref(other_it.bit_ref) {}

template<size_t Bits>
inline typename BitArray<Bits>::BitArrayRef& BitArray<Bits>::iterator::operator*() {
	if (*this >= bit_ref.ref_ptr->end()) {
		throw std::out_of_range("Out of range");
	}

	return bit_ref;
}

template<size_t Bits>
inline typename BitArray<Bits>::iterator& BitArray<Bits>::iterator::operator++() {
	bit_ref.bit_index += Bits;
	
	if (bit_ref.bit_index >= 64) {
		bit_ref.place_ptr += 1;
		bit_ref.bit_index -= 64;
	}

	return *this;
}

template<size_t Bits>
inline typename BitArray<Bits>::iterator& BitArray<Bits>::iterator::operator--() {
	if (bit_ref.bit_index < Bits) {
		bit_ref.place_ptr -= 1;
		bit_ref.bit_index = 64 - (Bits - bit_ref.bit_index);
	}
	else {
		bit_ref.bit_index -= Bits;
	}

	return *this;
}

template<size_t Bits>
inline typename BitArray<Bits>::iterator& BitArray<Bits>::iterator::operator+=(const uint64_t& val) {
	const size_t bits = bit_ref.bit_index + val * Bits;
	bit_ref.place_ptr += bits / 64;
	bit_ref.bit_index = bits % 64;

	return *this;
}

template<size_t Bits>
inline typename BitArray<Bits>::iterator& BitArray<Bits>::iterator::operator-=(const uint64_t& val) {
	const size_t bits = val * Bits;
	if (bits <= bit_ref.bit_index) {
		bit_ref.bit_index -= bits;
	}
	else {
		const size_t diff = bits - bit_ref.bit_index;

		bit_ref.place_ptr -= (diff + 63) / 64;
		bit_ref.bit_index = 64 - (diff % 64);
		if (bit_ref.bit_index == 64) {
			bit_ref.bit_index = 0;
		}
	}

	return *this;
}

template<size_t Bits>
inline typename BitArray<Bits>::iterator& BitArray<Bits>::iterator::operator=(const BitArray<Bits>::iterator& other_it) {
	bit_ref = other_it.bit_ref;

	return *this;
}

template<size_t Bits>
inline typename BitArray<Bits>::iterator BitArray<Bits>::iterator::operator+(size_t value) const {
	const size_t shift = bit_ref.bit_index + value * Bits;
	return BitArray<Bits>::iterator(bit_ref.ref_ptr, bit_ref.place_ptr + shift / 64, shift % 64);
}

template<size_t Bits>
inline typename BitArray<Bits>::iterator BitArray<Bits>::iterator::operator-(size_t value) const {
	BitArray<Bits>::iterator it(bit_ref.ref_ptr, bit_ref.place_ptr, bit_ref.bit_index);
	const size_t shift = value * Bits;
	if (shift <= bit_ref.bit_index) {
		it.bit_ref.bit_index -= shift;
	}
	else {
		const size_t diff = shift - bit_ref.bit_index;

		it.bit_ref.place_ptr -= (diff + 63) / 64;
		it.bit_ref.bit_index = 64 - (diff % 64);
		if (it.bit_ref.bit_index == 64) {
			it.bit_ref.bit_index = 0;
		}
	}

	return it;
}

template<size_t Bits>
inline bool BitArray<Bits>::iterator::operator==(const BitArray<Bits>::iterator& other_it) const {
	return bit_ref == other_it.bit_ref;
}

template<size_t Bits>
inline bool BitArray<Bits>::iterator::operator!=(const BitArray<Bits>::iterator& other_it) const {
	return bit_ref != other_it.bit_ref;
}

template<size_t Bits>
inline bool BitArray<Bits>::iterator::operator<(const BitArray<Bits>::iterator& other_it) const {
	assert(bit_ref.ref_ptr == other_it.bit_ref.ref_ptr);

	return bit_ref.place_ptr < other_it.bit_ref.place_ptr
		|| (bit_ref.place_ptr == other_it.bit_ref.place_ptr
			&& bit_ref.bit_index < other_it.bit_ref.bit_index);
}

template<size_t Bits>
inline bool BitArray<Bits>::iterator::operator>(const BitArray<Bits>::iterator& other_it) const {
	return other_it < *this;
}

template<size_t Bits>
inline bool BitArray<Bits>::iterator::operator<=(const BitArray<Bits>::iterator& other_it) const {
	return !(other_it < *this);
}

template<size_t Bits>
inline bool BitArray<Bits>::iterator::operator>=(const BitArray<Bits>::iterator& other_it) const {
	return !(*this < other_it);
}

#endif