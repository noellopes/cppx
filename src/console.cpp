/*
	Noel Lopes is a Professor at the Polytechnic of Guarda, Portugal
	and a Researcher at the CISUC - University of Coimbra, Portugal
	Copyright (C) 2017 Noel de Jesus Mendonça Lopes

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "console.h"
#include "rang.hpp"

namespace cppx {
	Console::Stream Console::current_stream = Console::Stream::error;

	std::ostream & Console::ErrorStream() {
		if (current_stream != Stream::error) {
			current_stream = Stream::error;
			std::cerr << rang::fgB::red;
		}

		return std::cerr;
	}

	std::ostream & Console::WarningStream() {
		if (current_stream != Stream::warning) {
			current_stream = Stream::warning;
			std::cerr << rang::fgB::yellow;
		}

		return std::cerr;
	}

	std::ostream & Console::OutputStream() {
		if (current_stream != Stream::standard) {
			current_stream = Stream::standard;
			std::cout << rang::style::reset;
		}

		return std::cout;
	}
}