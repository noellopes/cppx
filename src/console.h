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

#ifndef CPPX_CONSOLE_H
#define CPPX_CONSOLE_H

#include <iostream>

namespace cppx {

	/// Console class that provides three output streams 
	/// ([default] Output, Warning and Error), using appropriate 
	/// terminal colors for each one.
	/// Uses the rang c++ library for colors in the terminal 
	/// (https://github.com/agauniyal/rang)
	class Console {

	private:

		enum class Stream {
			standard,
			warning,
			error
		};

		static Stream current_stream;

	public:

		/// Returns an error stream (std::cerr) that will use write red text on the terminal.
		/// \return Error stream.
		static std::ostream & ErrorStream();

		/// Returns an error stream (std::cerr) that will use write yellow text on the terminal.
		/// \return Warning stream.
		static std::ostream & WarningStream();

		/// Returns the default output stream (std::cout).
		/// \return Default output stream.
		static std::ostream & OutputStream();
	};
}

#endif