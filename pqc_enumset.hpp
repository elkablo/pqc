#ifndef PQC_ENUMSET_HPP
#define PQC_ENUMSET_HPP

#include <iterator>
#include <pqc_enums.hpp>

namespace pqc
{

template<typename Enum, Enum _first, Enum _last>
class enumset
{
	typedef unsigned long word_t;
	static constexpr int values = _last - _first + 1;
	static constexpr int bits_per_word = sizeof(word_t)*8;
	static constexpr int nwords = (values + bits_per_word - 1) / bits_per_word;

	constexpr int word(Enum val) const {
		return (val - _first) / bits_per_word;
	}
	constexpr int wordbit(Enum val) const {
		return (val - _first) % bits_per_word;
	}
	constexpr word_t bit(Enum val) const {
		return 1 << wordbit(val);
	}
public:
	static constexpr Enum first = _first, last = _last;

	constexpr enumset()
	{
		clear();
	}

	template<typename... Args>
	constexpr enumset(Enum val, Args... args) : enumset(args...)
	{
		set(val);
	}

	constexpr void clear()
	{
		for (int i = 0; i < nwords; ++i)
			words_[i] = 0;
	}
	constexpr bool isset(Enum val) const
	{
		if (val >= _first && val <= _last)
			return words_[word(val)] & bit(val);
		else
			return false;
	}
	constexpr void set(Enum val, bool b = true)
	{
		if (val >= _first && val <= _last) {
			if (b)
				words_[word(val)] |= bit(val);
			else
				words_[word(val)] &= ~bit(val);
		}
	}
	constexpr void unset(Enum val)
	{
		set(val, false);
	}

	constexpr enumset& operator&(const enumset& other)
	{
		for (int i = 0; i < nwords; ++i)
			words_[i] &= other.words_[i];
		return *this;
	}

	constexpr enumset& operator|(const enumset& other)
	{
		for (int i = 0; i < nwords; ++i)
			words_[i] |= other.words_[i];
		return *this;
	}

	constexpr enumset operator~() const
	{
		enumset res;
		for (int i = 0; i < nwords; ++i)
			res.words_[i] = ~words_[i];
		return res;
	}

	constexpr bool operator!() const
	{
		for (int i = 0; i < nwords-1; ++i)
			if (words_[i])
				return false;
		return !(words_[nwords-1] << (bits_per_word - wordbit(last) + 1));
	}

	constexpr operator bool() const
	{
		return !*this;
	}

	class iterator : public std::iterator<std::forward_iterator_tag, Enum>
	{
	public:
		iterator() = delete;
		constexpr iterator(const enumset& set, Enum cur) : cur_(cur), set_(set) {}
		constexpr iterator(const enumset& set) : set_(set)
		{
			int i = (int) set_.last + 1;
			for (i = (int) set_.first; i <= (int) set_.last; ++i)
				if (set_.isset((Enum) i))
					break;
			cur_ = (Enum) i;
		}
		constexpr Enum operator*() const
		{
			return cur_;
		}
		constexpr iterator& operator++()
		{
			int i = (int) cur_;
			for (++i; i <= (int) set_.last; ++i)
				if (set_.isset((Enum) i))
					break;
			cur_ = (Enum) i;
		}
		constexpr iterator operator++(int) const
		{
			iterator res(*this);
			++res;
			return res;
		}
		constexpr bool operator==(const iterator& other) const
		{
			return &set_ == &other.set_ && cur_ == other.cur_;
		}
		constexpr bool operator!=(const iterator& other) const
		{
			return &set_ != &other.set_ || cur_ != other.cur_;
		}
	private:
		Enum cur_;
		const enumset& set_;
	};

	constexpr iterator begin()
	{
		return iterator(*this);
	}

	constexpr iterator end()
	{
		return iterator(*this, (Enum) ((int) last + 1));
	}
private:
	word_t words_[nwords];
};

typedef enumset<enum pqc_cipher, PQC_CIPHER_FIRST, PQC_CIPHER_LAST> cipherset;
typedef enumset<enum pqc_mac, PQC_MAC_FIRST, PQC_MAC_LAST> macset;
typedef enumset<enum pqc_kex, PQC_KEX_FIRST, PQC_KEX_LAST> kexset;
typedef enumset<enum pqc_auth, PQC_AUTH_FIRST, PQC_AUTH_LAST> authset;

}

#endif /* PQC_ENUMSET_HPP */
