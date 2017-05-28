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
#include <fstream>
#include <sstream>
#include <vector>
#include <boost/filesystem.hpp>

#include "console.h"
#include "parser.h"

using namespace cppx;
using namespace std;

/// Returns the extended C++ files to process.
/// \param base_dir Base directory where to look for the .cppx files. 
///        Its subdirectories will also be scanned for .cppx files.
/// \param files_to_process Vector that will be appended with the files 
///        to process contained in the \a base_dir directory and its 
///        subdirectories.
/// \return A vector containing the .cppx files to process
vector<boost::filesystem::path> GetFilesToProcess(const char * base_dir) {
	using namespace boost::filesystem;

	vector<path> files_to_process;

	try {
		for (const auto & entry : recursive_directory_iterator(base_dir)) {
			auto path = entry.path();
			if (path.extension() == ".cppx" && is_regular_file(path)) {
				files_to_process.push_back(path);
			}
		}
	} catch (const filesystem_error & exception) {
		Console::ErrorStream() << "An error ocurred while obtaining the files to process: " << exception.what() << endl;
	}

	return files_to_process;
}

std::string CodeGuardNamespaces(const std::vector<Parser::CodeBlock> & code_blocks) {
	std::string result;

	auto block = code_blocks.cbegin();

	auto ProcessNamespace = [&] {
		std::string namespace_identifer;

		while (++block != code_blocks.cend()) {
			switch (block->type) {
				case Parser::CodeBlock::Type::begin_group:
					result += namespace_identifer;
					return;

				case Parser::CodeBlock::Type::statement_terminator:
					return;

				case Parser::CodeBlock::Type::identifier:
					namespace_identifer += block->ToString() + "_";
					break;

				default:
					break;
			}
		}
	};

	for (; block != code_blocks.cend(); ++block) {
		switch (block->type) {
			case Parser::CodeBlock::Type::namespace_keyword:
				ProcessNamespace();
				break;

			case Parser::CodeBlock::Type::enumeration:
			case Parser::CodeBlock::Type::class_keyword:
			case Parser::CodeBlock::Type::struct_keyword:
				while (++block != code_blocks.cend()) {
					if (block->type == Parser::CodeBlock::Type::begin_group) {
						return result;
					} else if (block->type == Parser::CodeBlock::Type::statement_terminator) {
						break;
					}
				}
				break;

			default:
				break;
		}
	}

	return result;
}

std::string CodeGuardIdentifier(const boost::filesystem::path & path, const std::vector<Parser::CodeBlock> & code_blocks) {
	std::string result = CodeGuardNamespaces(code_blocks);

	result += path.stem().string() + "_H";

	for (auto & c : result) c = toupper(c);

	return result;
}

class CodeWriter {
private:
	std::ofstream cpp_file;
	std::ofstream header_file;

	std::string buffer;

public:
	CodeWriter(const std::string & cpp_filename, const std::string & header_filename) :
		cpp_file(cpp_filename, std::ofstream::binary),
		header_file(header_filename, std::ofstream::binary) {
	}

	CodeWriter(const CodeWriter &) = delete;
	CodeWriter & operator = (const CodeWriter &) = delete;

	~CodeWriter() {
		WriteBufferToHeader();
	}

	void AppendToBuffer(std::string & s) {
		buffer += s;
	}

	void WriteBufferToHeader() {
		if (buffer.size() > 0) {
			header_file << buffer;
			buffer.clear();
		}
	}

	void WriteBufferToCpp() {
		if (buffer.size() > 0) {
			cpp_file << buffer;
			buffer.clear();
		}
	}

	void WriteBufferToBoth() {
		if (buffer.size() > 0) {
			header_file << buffer;
			cpp_file << buffer;
			buffer.clear();
		}
	}

	void WriteToHeader(const std::string & s) {
		WriteBufferToHeader();
		header_file << s;
	}

	void WriteToCpp(const std::string & s) {
		WriteBufferToCpp();
		cpp_file << s;
	}


	void WriteToBoth(const std::string & s) {
		WriteBufferToBoth();
		header_file << s;
		cpp_file << s;
	}

	std::ofstream & HeaderFile() {
		WriteBufferToHeader();
		return header_file;
	}

	std::ofstream & CppFile() {
		WriteBufferToCpp();
		return cpp_file;
	}
};

/// Generates C++ code (.h and .cpp files) from a specific extended C++ file (.cppx)
/// \param filename filename (.cppx) to process
/// \sa GenerateCode
void GenerateFileCode(const boost::filesystem::path & filename) {
	using namespace boost::filesystem;

	try {
		Parser parser(filename);

		auto code_blocks = parser.CodeBlocks();

		if (code_blocks.empty()) {
			Console::WarningStream() << "File " << filename << " does not contain any code to process" << endl;
			return;
		}

		std::string header_filename = path(filename).replace_extension("h").string();
		CodeWriter code_writer(path(filename).replace_extension("cpp").string(), header_filename);

		auto code_block = code_blocks.cbegin();

		if (code_block->type == Parser::CodeBlock::Type::comment) {
			code_writer.WriteToBoth(code_block->ToString());
			code_block++;
		}

		std::string include_guard = CodeGuardIdentifier(filename, code_blocks);

		code_writer.HeaderFile() << "#ifndef " << include_guard << std::endl;
		code_writer.HeaderFile() << "#define " << include_guard << std::endl << std::endl;

		code_writer.CppFile() << "#include \"" << header_filename << '"' << std::endl << std::endl;

		Parser::Container::Type next_container = Parser::Container::Type::none;
		std::vector<Parser::Container> containers { Parser::Container(next_container) };
		
		while (++code_block != code_blocks.cend()) {
			std::string current_code = code_block->ToString();

			auto ProcessContainer = [&] {
				std::string identifier;

				code_writer.AppendToBuffer(current_code);

				while (++code_block != code_blocks.cend()) {
					current_code = code_block->ToString();
					code_writer.AppendToBuffer(current_code);

					switch (code_block->type) {
						case Parser::CodeBlock::Type::identifier:
							if (identifier.empty()) identifier = current_code;
							break;

						case Parser::CodeBlock::Type::begin_group:
							containers.push_back(Parser::Container(identifier, next_container, 1));
							code_writer.WriteBufferToHeader();
							return;

						case Parser::CodeBlock::Type::statement_terminator:
							code_writer.WriteBufferToHeader();
							return;

						default:
							break;
					}
				}
			};

			switch (code_block->type) {
				case cppx::Parser::CodeBlock::Type::directive:
				case cppx::Parser::CodeBlock::Type::access_modifier:
					code_writer.WriteToHeader(current_code);
					break;

				case cppx::Parser::CodeBlock::Type::namespace_keyword:
					next_container = Parser::Container::Type::namespace_container;
					ProcessContainer();
					break;

				case cppx::Parser::CodeBlock::Type::class_keyword:
					next_container = Parser::Container::Type::class_container;
					ProcessContainer();
					break;

				case cppx::Parser::CodeBlock::Type::struct_keyword:
					next_container = Parser::Container::Type::struct_container;
					ProcessContainer();
					break;

				case cppx::Parser::CodeBlock::Type::enumeration:
					next_container = Parser::Container::Type::enumeration;
					ProcessContainer();
					break;

				case cppx::Parser::CodeBlock::Type::function_name:
				case cppx::Parser::CodeBlock::Type::constructor_destructor:
					{
						[&] {
							std::string function_name = current_code;
							std::string function = current_code;
							
							while (++code_block != code_blocks.cend()) {
								current_code = code_block->ToString();

								switch (code_block->type) {
									case Parser::CodeBlock::Type::begin_group:

									case Parser::CodeBlock::Type::initialization_list:									
										code_writer.WriteBufferToBoth();

										// add the scope to cpp file
										for (const auto & c : containers) {
											if (c.name.size() > 0) code_writer.WriteToCpp(c.name + "::");
										}

										code_writer.WriteToBoth(function);

										code_writer.HeaderFile() << ';';

										code_writer.WriteToCpp(current_code);										

										containers.push_back(Parser::Container(function_name, Parser::Container::Type::function, (code_block->type == Parser::CodeBlock::Type::begin_group) ? 1 : 0));

										while (containers.back().type == Parser::Container::Type::function && ++code_block != code_blocks.cend()) {
											code_writer.WriteToCpp(code_block->ToString());

											switch (code_block->type) {
												case Parser::CodeBlock::Type::begin_group:
													containers.back().braces++;
													break;

												case Parser::CodeBlock::Type::end_group:
													if (--(containers.back().braces) == 0) containers.pop_back();
													break;

												default:
													break;
											}
										}

										return;

									case Parser::CodeBlock::Type::statement_terminator:
										code_writer.WriteToHeader(function + current_code);
										return;

									default:
										function += current_code;
										break;
								}
							}
						} ();
					}
					break;

				case cppx::Parser::CodeBlock::Type::statement_terminator:
					code_writer.WriteToHeader(current_code);
					break;

				case cppx::Parser::CodeBlock::Type::end_group:
					code_writer.WriteToHeader(current_code);

					if (--(containers.back().braces) == 0) {
						if (containers.size() > 1) containers.pop_back();
					}
					break;

				case cppx::Parser::CodeBlock::Type::begin_group:
					code_writer.WriteToHeader(current_code);
					containers.back().braces++;
					break;

				default:
					code_writer.AppendToBuffer(current_code);
					break;
			}
		}

		code_writer.HeaderFile() << std::endl << std::endl << "#endif // " << include_guard << std::endl << std::endl;
	} catch (const Parser::Error & error) {
		Console::ErrorStream() << "Error at " << filename << " (line " << error.Line() << "): ";
		Console::ErrorStream() << error.what() << ": " << error.CodeContainingError() << std::endl;
	}
}

/// Generates C++ code (.h and .cpp files) from all extended C++ files (.cppx) contained within the 
/// \a base_dir directory and its subdirectories
/// \param base_dir base directory for processing the .cppx files
/// \return 0 if successfull. An error code otherwise.
/// \sa GenerateFileCode
int GenerateCode(const char * base_dir) {
	using namespace boost::filesystem;

	constexpr int OK_RESULT = 0;
	constexpr int ERROR_RESULT = 1;

	try {
		if (!exists(base_dir) || !is_directory(base_dir)) {
			Console::ErrorStream() << "Could not access directory: " << base_dir << endl;
			return ERROR_RESULT;
		}
	} catch (const filesystem_error & exception) {
		Console::ErrorStream() << "An error ocurred while accessing directory '" << base_dir << "': " << exception.what() << endl;
		return ERROR_RESULT;
	}

	Console::OutputStream() << "Processing directory: " << base_dir << endl;

	vector<path> files_to_process = GetFilesToProcess(base_dir);

	auto number_files_to_process = files_to_process.size();

	if (files_to_process.size() == 0) {
		Console::WarningStream() << "No extend C++ files (.cppx) found in '" << base_dir << "' or in its subdirectories" << endl;
	} else {
		Console::OutputStream() << "Found " << number_files_to_process << " files to process:" << endl;

		for (const path & f : files_to_process) {
			boost::system::error_code error;
			auto size = file_size(f, error);

			Console::OutputStream() << f;

			if (!error) {
				Console::OutputStream() << " (" << size << " bytes)";
			}

			Console::OutputStream() << endl;

			GenerateFileCode(f);
		}
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
