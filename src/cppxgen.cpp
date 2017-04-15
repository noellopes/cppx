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

/// \file cppxgen.cpp
/// cppxgen tool: converts extended C++ files (.cppx) to standard C++ files (.h and .cpp)

#include <iostream>
#include <vector>
#include <boost/filesystem.hpp>

#include "console.h"

using namespace cppx;
using namespace std;

/// Function for obtaining the extended C++ files to process.
/// \param base_dir Base directory where to look for the .cppx files. 
///        Its subdirectories will also be scanned for .cppx files.
/// \param files_to_process Vector that will be appended with the files 
///        to process contained in the \a base_dir directory and its 
///        subdirectories.
void GetFilesToProcess(const boost::filesystem::path & base_dir, std::vector<boost::filesystem::path> & files_to_process) {
	using namespace boost::filesystem;

	try {
		for (const auto & entry : recursive_directory_iterator(base_dir)) {
			auto path = entry.path();
			if (path.extension() == ".cppx") {
				files_to_process.push_back(path);
			}
		}
	} catch (const filesystem_error& exception) {
		Console::ErrorStream() << "An error ocurred while obtaining the files to process: " << exception.what() << endl;
	}
}

/// Generates C++ code (.h and .cpp files) from all extended C++ files (.cppx) contained within the 
/// \a base_dir directory and its subdirectories
/// \param base_dir base directory for processing the .cppx files
/// \return 0 if successfull. An error code otherwise.
/// \sa GenerateCodeForDirectory
int GenerateCode(const char * base_dir) {
	using namespace boost::filesystem;

	constexpr int OK_RESULT = 0;
	constexpr int ERROR_RESULT = 1;

	try {
		if (!exists(base_dir) || !is_directory(base_dir)) {
			Console::ErrorStream() << "Could not access directory: " << base_dir << endl;
			return ERROR_RESULT;
		}
	} catch (const filesystem_error& exception) {
		Console::ErrorStream() << "An error ocurred while accessing directory '" << base_dir << "': " << exception.what() << endl;
		return ERROR_RESULT;
	}

	Console::OutputStream() << "Processing directory: " << base_dir << endl;

	vector<path> files_to_process;
	GetFilesToProcess(path(base_dir), files_to_process);

	auto number_files_to_process = files_to_process.size();

	if (files_to_process.size() == 0) {
		Console::WarningStream() << "No extend C++ files (.cppx) found in '" << base_dir << "' or in its subdirectories" << endl;
	} else {
		Console::OutputStream() << "Found " << number_files_to_process << " files to process:" << endl;

		for (const auto & f : files_to_process) {
			Console::OutputStream() << f << "(" << file_size(f) << " bytes)" << endl;
		}

		Console::OutputStream() << endl << "We have yet to develop the code generators' modules." << endl;
	}

	return OK_RESULT;
}

/// Main entry point for cppxgen tool
/// Converts extended C++ files (.cppx) to standard C++ files (.h and .cpp).
/// Usage: cppxgen [base directory (default current)]
int main(int argc, char * argv[]) {
	Console::OutputStream() << CPPX_VERSION_STRING << endl;
	Console::OutputStream() << "Converts extended C++ files (.cppx) to standard C++ files (.h and .cpp)" << endl;
	Console::OutputStream() << "Usage: cppxgen [base directory (default current)]" << endl;
	Console::OutputStream() << endl;

	int error_code = GenerateCode((argc < 2) ? "./" : argv[1]);

	Console::OutputStream() << endl;
	Console::OutputStream() << "Thank you for trying cppxgen." << endl;

	return error_code;
}
