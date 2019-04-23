#pragma once

#include <streambuf>
#include <type_traits>
#include <chrono>

namespace inet
{
	// Generic streambuf for use with sockets and the like, seeking is not supported, and has an additional reset function
    template<typename CharT, typename Traits = std::char_traits<CharT>>
	class gstreambuf : public std::basic_streambuf<CharT, Traits>
	{
		static constexpr int _isize = 18*1024;
		static constexpr int _osize = 16*1024;
		static constexpr int _putback =2*1024;

	public:

		typedef CharT							char_type;
		typedef Traits							traits_type;
		typedef typename traits_type::int_type	int_type;
		typedef typename traits_type::pos_type	pos_type;
		typedef typename traits_type::off_type	off_type;

		template<typename T>
		explicit gstreambuf(T t);

		gstreambuf();

		gstreambuf(const gstreambuf& rhs) = delete;

		virtual ~gstreambuf();

		gstreambuf& operator=(const gstreambuf& rhs) = delete;

		template<typename T>
		gstreambuf& reset(T t);

		gstreambuf& reset();

		// Limits

		void enable_timeout(unsigned int ms);

		void reset_timeout();

		void disable_timeout();

		void enable_data_limit(unsigned int count);

		void reset_data_limit();

		void disable_data_limit();

	protected:

		typedef std::chrono::duration<int, std::ratio<1, 1000>> milisecond;

		virtual void readfunc(const void * const t, size_t& res, char *begin, size_t len) = 0;

		virtual void writefunc(const void * const t, size_t& res, char *begin, size_t len) = 0;

		std::chrono::high_resolution_clock::time_point _begin;

		int _duration = -1;

		unsigned int _curread;

		unsigned int _maxread;

		bool _inlimit = false;
		
	private:

		// Positioning

		virtual int sync() override;

		// Get area

		std::streamsize showmanyc() override;

		int_type underflow() override;

		int_type uflow() override;

		std::streamsize xsgetn(char_type *s, std::streamsize count) override;

		// Put area

		std::streamsize xsputn(const char_type *s, std::streamsize count) override;

		int_type overflow(int_type ch = traits_type::eof()) override;

		// Putback

		int_type pbackfail(int_type ch) override;

		// Data members

		void *_if;

		char_type _ibuffer[_isize];
		char_type *_icur, *_iend;

		char_type _obuffer[_osize];
		char_type *_ocur;
	};

	template<typename CharT, typename Traits>
	template<typename T>
	inline gstreambuf<CharT, Traits>::gstreambuf(T t)
		: _icur(_ibuffer), _iend(_ibuffer), _ocur(_obuffer)
	{
		static_assert(std::is_fundamental_v<T> || std::is_pointer_v<T>, "Fundamentals and pointers only!");
		_if = std::malloc(sizeof(T));
		*static_cast<T*>(_if) = t;
		this->setg(nullptr, nullptr, nullptr);
		this->setp(nullptr, nullptr);
	}

	template<typename CharT, typename Traits>
	inline gstreambuf<CharT, Traits>::gstreambuf()
		: _if(nullptr), _icur(_ibuffer), _iend(_ibuffer), _ocur(_obuffer)
	{
		this->setg(nullptr, nullptr, nullptr);
		this->setp(nullptr, nullptr);
	}

	template<typename CharT, typename Traits>
	inline gstreambuf<CharT, Traits>::~gstreambuf()
	{
		free(_if);
	}

	template<typename CharT, typename Traits>
	template<typename T>
	inline gstreambuf<CharT, Traits>& gstreambuf<CharT, Traits>::reset(T t)
	{
		static_assert(std::is_fundamental_v<T> || std::is_pointer_v<T>, "Fundamentals and pointers only!");
		_icur = _iend = _ibuffer;
		_ocur = _obuffer;
		if (_if == nullptr) {
			_if = std::malloc(sizeof(T));
			*static_cast<T*>(_if) = t;
		}
		else
			*static_cast<T*>(_if) = t;
		return *this;
	}

	template<typename CharT, typename Traits>
	inline gstreambuf<CharT, Traits>& gstreambuf<CharT, Traits>::reset()
	{
		_icur = _iend = _ibuffer;
		_ocur = _obuffer;
		if (_if != nullptr) {
			free(_if);
			_if = nullptr;
		}
		return *this;
	}

	template<typename CharT, typename Traits>
	inline void inet::gstreambuf<CharT, Traits>::enable_timeout(unsigned int ms)
	{
		_duration = static_cast<int>(ms);
		_begin = std::chrono::high_resolution_clock::now();
	}

	template<typename CharT, typename Traits>
	inline void inet::gstreambuf<CharT, Traits>::reset_timeout()
	{
		_begin = std::chrono::high_resolution_clock::now();
	}

	template<typename CharT, typename Traits>
	inline void gstreambuf<CharT, Traits>::disable_timeout()
	{
		_duration = -1;
	}

	template<typename CharT, typename Traits>
	inline void inet::gstreambuf<CharT, Traits>::enable_data_limit(unsigned int count)
	{
		_curread = 0;
		_maxread = count;
		_inlimit = true;
	}

	template<typename CharT, typename Traits>
	inline void inet::gstreambuf<CharT, Traits>::reset_data_limit()
	{
		_curread = 0;
	}

	template<typename CharT, typename Traits>
	inline void gstreambuf<CharT, Traits>::disable_data_limit()
	{
		_inlimit = false;
	}

	template<typename CharT, typename Traits>
	inline int gstreambuf<CharT, Traits>::sync()
	{
		auto size = (reinterpret_cast<uintptr_t>(_ocur) - reinterpret_cast<uintptr_t>(_obuffer))/sizeof(char_type);
		if (size > 0) {
			size_t out = 0;
			do {
				auto old = out;
				writefunc(_if, out, _obuffer+out, size-out);
				if (old == out) {
					return -1;			
				}
			}
			while (out != size);
			_ocur = _obuffer;
		}
		return 0;
	}

	template<typename CharT, typename Traits>
	inline std::streamsize gstreambuf<CharT, Traits>::showmanyc()
	{
		return (reinterpret_cast<uintptr_t>(_iend) - reinterpret_cast<uintptr_t>(_ibuffer))/sizeof(char_type);
	}

	template<typename CharT, typename Traits>
	inline typename gstreambuf<CharT, Traits>::int_type gstreambuf<CharT, Traits>::underflow()
	{
		if (_icur < _iend)
			return traits_type::to_int_type(*_icur);

		if (_if == nullptr)
			return traits_type::eof();

		// Calculate the current space used in buffer
		auto delta = (reinterpret_cast<uintptr_t>(_icur) - reinterpret_cast<uintptr_t>(_ibuffer))/sizeof(char_type);

		// Move memory if there is no more space left
		if (delta == _isize && _iend != _ibuffer) {
			std::copy(_ibuffer+_isize-_putback, _ibuffer+_isize, _ibuffer);
			_icur = _ibuffer+_putback;
			delta = _putback;
		}

		// Read
		size_t n = 0;
		readfunc(_if, n, _ibuffer+delta, _isize-delta);
		if (n == 0)
			return traits_type::eof();
		_iend = _ibuffer+delta+n;

		return traits_type::to_int_type(*_icur);
	}

	template<typename CharT, typename Traits>
	inline typename gstreambuf<CharT, Traits>::int_type gstreambuf<CharT, Traits>::uflow()
	{
		if (_icur == _iend) {
			auto ret = underflow();
			++_icur;
			return ret;
		}
		else
			return traits_type::to_int_type(*_icur++);
	}

	template<typename CharT, typename Traits>
	inline std::streamsize gstreambuf<CharT, Traits>::xsgetn(char_type *s, std::streamsize count)
	{
		std::streamsize i = 0;
		while (true) {
			while (_icur != _iend && i < count)
				s[i++] = *_icur++;
			if (i != count) {
				if (underflow() == traits_type::eof())
					break;
			}
			else
				break;
		}
		return i;
	}

	template<typename CharT, typename Traits>
	inline std::streamsize gstreambuf<CharT, Traits>::xsputn(const char_type * s, std::streamsize count)
	{
		if (_if == nullptr)
			return 0;

		std::streamsize i = 0;
		while (true) {
			while (_ocur != _obuffer+_osize && i < count)
				*_ocur++ = s[i++];
			if (i != count) {
				if (overflow() == traits_type::eof())
					break;
			}
			else 
				break;
		}
		return i;
	}

	template<typename CharT, typename Traits>
	inline typename gstreambuf<CharT, Traits>::int_type gstreambuf<CharT, Traits>::overflow(int_type ch)
	{
		if (ch == traits_type::eof())
			return 0;

		if (_ocur == _obuffer+_osize) {
			if (sync() == -1)
				return traits_type::eof();
		}

		if (_if == nullptr)
			return traits_type::eof();

		*_ocur++ = traits_type::to_char_type(ch);
		return 0;
	}

	template<typename CharT, typename Traits>
	inline typename gstreambuf<CharT, Traits>::int_type gstreambuf<CharT, Traits>::pbackfail(int_type ch)
	{
		if (_icur == _ibuffer)
			return traits_type::eof();
		else if (ch == traits_type::eof())
			return traits_type::to_int_type(*--_icur);
		else {
			*--_icur = traits_type::to_char_type(ch);
			return *_icur;
		}
	}
}