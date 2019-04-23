#pragma once

#include "gstreambuf.hpp"
#include <istream>
#include <string>
#include <chrono>
#include <type_traits>

namespace inet
{
    // Generic connection class for use in high level networking functions (like http clients and servers)
    template<typename CharT, typename Traits = std::char_traits<CharT>>
	class gconnection : public std::basic_iostream<CharT, Traits>
	{
	public:

		gconnection()
			: std::basic_iostream<CharT, Traits>(nullptr)
		{
		}

		gconnection(const gconnection& rhs) = delete;

		gconnection(gconnection&& rhs) = delete;

		virtual ~gconnection()
		{
			this->set_rdbuf(nullptr);
			delete _sb;
		}

		// fstream like functions

		virtual bool is_open() const noexcept = 0;

		virtual void open(std::string_view node, std::string_view service) = 0;

		virtual void close() = 0;

		// connection functions

		void enable_timeout(unsigned int ms)
		{
			_sb->enable_timeout(ms);
		}

		void reset_timeout()
		{
			_sb->reset_timeout();
		}

		void disable_timeout()
		{
			_sb->disable_timeout();
		}

		void enable_read_limit(unsigned int bytes)
		{
			_sb->enable_data_limit(bytes);
		}

		void reset_read_limit()
		{
			_sb->reset_data_limit();
		}

		void disable_read_limit()
		{
			_sb->disable_data_limit();
		}

		virtual const char * getprotocol() = 0;

		// extra functionality
		virtual gconnection& getCRLF(std::basic_string<CharT, Traits, std::allocator<CharT>>& str)
		{
			bool rfound = false;
			while (true) {
				auto c = this->get();
				if (c == Traits::eof())
					break;
				else if (rfound) {
					if (c == '\n') {
						str.pop_back();
						break;
					}
					rfound = false;
				}
				else if (c == '\r')
					rfound = true;
				str.push_back(Traits::to_char_type(c));
			}
			return *this;
		}

	protected:

		bool _connected = false;

		gstreambuf<CharT, Traits> *_sb = nullptr;
	};
}