#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <map>

namespace json
{
	/*
	 * JSON exceptions
	*/
	enum class except_e { MALFORMED, CORRUPT, IO_ERROR };

	class exception : public std::exception
	{
	public:

		exception() = delete;

		exception(except_e ecode);

		const char * what() const noexcept override;

		const except_e ecode;
	};

	/*
	 * JSON type definitions
	*/
	enum class type_e { INT, UINT, DOUBLE, BOOL, STR, ARR, OBJ, NUL };

	class type_container;

	class array : public std::vector<type_container>
	{
	public:

		std::string toString() const;

		std::string toHumanString(unsigned int level = 0) const;
	};

	class object : public std::map<std::string, type_container>
	{
	public:

		std::string toString() const;

		std::string toHumanString(unsigned int level = 0) const;
	};

	/*
	 * Wrapper around JSON types to identify them
	*/
	template<type_e t>
	struct raw_type
	{
		raw_type() {}
		raw_type(std::nullptr_t rhs) noexcept;
		raw_type& operator=(const raw_type& rhs) noexcept;
		raw_type& operator=(std::nullptr_t rhs) noexcept;

		const type_e	_type = type_e::NUL;
		std::nullptr_t	_value;
	};

	template<>
	struct raw_type<type_e::INT>
	{
		raw_type() {}
		raw_type(long long rhs) noexcept;
		raw_type& operator=(const raw_type& rhs) noexcept;
		raw_type& operator=(long long rhs) noexcept;

		const type_e	_type = type_e::INT;
		long long		_value;
	};

	template<>
	struct raw_type<type_e::UINT>
	{
		raw_type() {}
		raw_type(unsigned long long rhs) noexcept;
		raw_type& operator=(const raw_type& rhs) noexcept;
		raw_type& operator=(unsigned long long rhs) noexcept;

		const type_e		_type = type_e::UINT;
		unsigned long long	_value;
	};

	template<>
	struct raw_type<type_e::DOUBLE>
	{
		raw_type() {}
		raw_type(double rhs) noexcept;
		raw_type& operator=(const raw_type& rhs) noexcept;
		raw_type& operator=(double rhs) noexcept;

		const type_e	_type = type_e::DOUBLE;
		double			_value;
	};

	template<>
	struct raw_type<type_e::BOOL>
	{
		raw_type() {}
		raw_type(bool rhs) noexcept;
		raw_type& operator=(const raw_type& rhs) noexcept;
		raw_type& operator=(bool rhs) noexcept;

		const type_e	_type = type_e::BOOL;
		bool			_value;
	};

	template<>
	struct raw_type<type_e::STR>
	{
		raw_type() {}
		raw_type(const std::string& rhs);
		raw_type(std::string&& rhs) noexcept;
		raw_type& operator=(const raw_type& rhs);
		raw_type& operator=(raw_type&& rhs) noexcept;
		raw_type& operator=(const std::string& rhs);
		raw_type& operator=(std::string&& rhs) noexcept;

		const type_e	_type = type_e::STR;
		std::string		_value;
	};

	template<>
	struct raw_type<type_e::ARR>
	{
		raw_type() {}
		raw_type(const array& rhs);
		raw_type(array&& rhs) noexcept;
		raw_type& operator=(const raw_type& rhs);
		raw_type& operator=(raw_type&& rhs) noexcept;
		raw_type& operator=(const array& rhs);
		raw_type& operator=(array&& rhs) noexcept;

		const type_e	_type = type_e::ARR;
		array			_value;
	};

	template<>
	struct raw_type<type_e::OBJ>
	{
		raw_type() {}
		raw_type(object&& rhs) noexcept;
		raw_type(const object& rhs);
		raw_type& operator=(const raw_type& rhs);
		raw_type& operator=(raw_type&& rhs) noexcept;
		raw_type& operator=(const object& rhs);
		raw_type& operator=(object&& rhs) noexcept;

		const type_e	_type = type_e::OBJ;
		object			_value;
	};

	/*
	 * A container to store a single JSON type
	*/
	class type_container
	{
	public:
		type_container() noexcept;

		type_container(const type_container& rhs);

		type_container(type_container&& rhs) noexcept;

		~type_container();

		type_container& operator=(const type_container& rhs);

		type_container& operator=(type_container&& rhs) noexcept;

		template<type_e Jt, class T>
		type_container& copy(const T& rhs);

		template<type_e Jt, class T>
		type_container& move(T&& rhs) noexcept;

		long long& getInt();

		const long long& getInt() const;

		unsigned long long& getUInt();

		const unsigned long long& getUInt() const;

		double& getDouble();

		const double& getDouble() const;

		std::string& getString();

		const std::string& getString() const;

		array& getArray();

		const array& getArray() const;

		object& getObject();

		const object& getObject() const;

		std::nullptr_t& getNull();

		const std::nullptr_t& getNull() const;

		type_e getType() const;

		std::string toString() const;

		std::string toHumanString(unsigned int level = 0) const;

	private:

		void _del();

		raw_type<type_e::NUL> *_data;
	};

	/*
	 * All template implementation
	*/
	template<type_e t>
	inline raw_type<t>::raw_type(std::nullptr_t rhs) noexcept
		: _value(rhs)
	{
	}

	template<type_e t>
	inline raw_type<t>& raw_type<t>::operator=(const raw_type& rhs) noexcept
	{
		_value = rhs._value;
		return *this;
	}

	template<type_e t>
	inline raw_type<t>& raw_type<t>::operator=(std::nullptr_t rhs) noexcept
	{
		_value = rhs;
		return *this;
	}

	template<type_e Jt, class T>
	inline type_container& type_container::copy(const T& rhs)
	{
		if (_data == nullptr) {
			*reinterpret_cast<raw_type<Jt>**>(&_data) = new raw_type<Jt>();
		}
		else if (_data->_type != Jt) {
			_del();
			*reinterpret_cast<raw_type<Jt>**>(&_data) = new raw_type<Jt>();
		}
		*reinterpret_cast<raw_type<Jt>*>(_data) = rhs;
		return *this;
	}

	template<type_e Jt, class T>
	inline type_container & type_container::move(T&& rhs) noexcept
	{
		if (_data != nullptr) {
			if (_data->_type == Jt) {
				*reinterpret_cast<raw_type<Jt>*>(_data) = std::move(rhs);
				return *this;
			}
			else {
				_del();
			}
		}
		*reinterpret_cast<raw_type<Jt>**>(&_data) = new raw_type<Jt>(std::move(rhs));
		return *this;
	}
}