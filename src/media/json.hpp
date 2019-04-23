#pragma once

#include "jsontypes.hpp"
#include "unicode.hpp"

namespace json
{
	class parser
	{
	public:

		type_container parseMemory(const char *begin, unsigned int size);

		type_container parseFile(std::string_view path);

		parser& disableExpensiveChecks(bool disable = true);

	private:

		void ignoreSpace();

		std::string extractStdString();

		type_container extractNumber();

		type_container extractBoolean();

		type_container extractString();

		type_container extractArray();

		type_container extractObject();

		type_container extractNull();

		bool _fast = false;

		const char *_cur, *_end;
	};
}