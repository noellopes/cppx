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

#ifndef CPPX_PARSER_H
#define CPPX_PARSER_H

#include <vector>
#include <regex>

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include "console.h"

namespace cppx {
	class Parser {
	private:
		class Iterator {
		private:
			const char * begin;
			const char * current;
			size_t line;

			void Advance(size_t length) {
				for (size_t i = 0; i < length; ++i) MoveNext();
			}

		public:
			explicit Iterator(const char * code) {
				begin = current = code;
				line = 1;
			}

			Iterator(const Iterator &) = default;

			Iterator & operator = (const Iterator &) = default;

			unsigned char Value() const {
				return *current;
			}

			unsigned char PreviousValue() const {
				return (current == begin) ? 0 : *(current - 1);
			}

			unsigned char Peek() const {
				return (*current) ? *(current + 1) : 0;
			}

			size_t Line() const {
				return line;
			}

			const char * Begin() const {
				return begin;
			}

			const char * Pointer() const {
				return current;
			}

			inline size_t Index() const {
				return current - begin;
			}

			void MoveBack(size_t lenght) {
				while (lenght-- > 0) MovePrevious();
			}

			void MoveNext() {
				if (!*current) return;

				if (*current++ == '\n') line++;
			}

			void MovePrevious() {
				if (current == begin) return;

				if (*current-- == '\n') line--;
			}

			unsigned char Next() {
				MoveNext();

				return *current;
			}

			bool Match(const char * regular_expression, std::cmatch & match_result) {
				if (std::regex_search(current, match_result, std::regex(regular_expression), std::regex_constants::match_continuous)) {
					Advance(match_result.length());
					return true;
				}

				return false;
			}

			bool Match(const char * regular_expression) {
				std::cmatch match_result;

				return Match(regular_expression, match_result);
			}

			bool AdvanceUntilCharIsFound(char c) {
				for (; *current; MoveNext()) {
					if (*current == c) return true;
				}

				return false;
			}

			bool AdvanceUntilCharIsFound(const char * possible_chars) {
				for (; *current; MoveNext()) {
					for (const char * c = possible_chars; *c; ++c) {
						if (*current == *c) return true;
					}
				}

				return false;
			}
		};

	public:
		struct Container {
		public:
			enum class Type {
				none,
				namespace_container,
				class_container,
				struct_container,
				enumeration,
				function,
				constructor_destructor,
				initialization_list
			};

			Type type;
			size_t braces;
			size_t parenthesis;
			std::string name;

			Container(const std::string & name, Parser::Container::Type type, size_t braces = 0) :
				type(type), braces(braces), parenthesis(0), name(name) {}

			Container(Parser::Container::Type type, size_t braces = 0) :
				type(type), braces(braces), parenthesis(0) {}
		};

		class Error : public std::runtime_error {
		private:
			static constexpr size_t NUMBER_CHARS_CODE_CONTAINING_ERROR = 28;

			size_t line;
			std::string code_containing_error;

			void SetCodeContainingError(const Iterator & iterator) {
				const char * code = iterator.Pointer();

				for (size_t i = 0; i < NUMBER_CHARS_CODE_CONTAINING_ERROR; ++i) {
					if (!*code || *code == '\n') break;
					code_containing_error += *code++;
				}
			}

		public:
			explicit Error(const char * error_message, const Iterator & iterator) :
				std::runtime_error(error_message), line(iterator.Line()) {
				SetCodeContainingError(iterator);
			}

			explicit Error(const std::ostringstream & error_message, const Iterator & iterator) :
				std::runtime_error(error_message.str()), line(iterator.Line()) {
				SetCodeContainingError(iterator);
			}

			size_t Line() const { return line; }

			const std::string & CodeContainingError() const { return code_containing_error; }
		};

		struct CodeBlock {
		public:
			enum class Type {
				none,
				empty,
				comment,
				directive,
				char_literal,
				string_literal,
				identifier,
				access_modifier,
				namespace_keyword,
				class_keyword,
				struct_keyword,
				enumeration,
				arguments_or_parameters,
				function_name,
				constructor_destructor,
				initialization_list,
				begin_group,
				end_group,
				statement_terminator,
				identifier_scope,
				previous_type,
				other
			};

			Type type;
			const char * begin;
			const char * end;

			CodeBlock(Type type, const char * begin, const char * end) :
				type(type), begin(begin), end(end) {}

			size_t Size() const {
				return end - begin + 1;
			}

			std::string ToString() const {
				return std::string(begin, Size());
			}
		};

	private:
		boost::iostreams::mapped_file code;
		std::vector<CodeBlock> code_blocks;
		std::vector<Container> containers;
		Iterator iterator;

		// Excludes non code blocks
		class CodeBlockReverseIterator {
		private:
			typename std::vector<CodeBlock>::reverse_iterator current;
			typename std::vector<CodeBlock>::reverse_iterator end;
			size_t skipped;

		public:
			explicit CodeBlockReverseIterator(std::vector<CodeBlock> & code_blocks) {
				skipped = 0;

				end = code_blocks.rend();
				for (current = code_blocks.rbegin(); current != end; ++current) {
					switch (current->type) {
						case CodeBlock::Type::none:
						case CodeBlock::Type::empty:
						case CodeBlock::Type::comment:
							skipped++;
							break;

						default:
							return;
					}
				}
			}

			size_t Skipped() const {
				return skipped;
			}

			auto Current() const -> decltype(current) {
				return current;
			}

			auto End() const -> decltype(end) {
				return end;
			}

			auto Next() -> decltype(current) {
				while (++current != end) {
					switch (current->type) {
						case CodeBlock::Type::none:
						case CodeBlock::Type::empty:
						case CodeBlock::Type::comment:
							skipped++;
							break;

						default:
							return current;
					}
				}

				return current;
			}

			CodeBlock::Type CurrentType() const {
				if (current == end) return CodeBlock::Type::none;
				return current->type;
			}
		};

		bool MergeCodeBlocks(const CodeBlock & code_block) {
			if (code_blocks.empty()) return false;

			size_t blocks_to_merge = 0;

			switch (code_block.type) {
				case CodeBlock::Type::begin_group:
					if (code_blocks.back().type == CodeBlock::Type::empty) {
						blocks_to_merge = 1;
					}

					break;

				case CodeBlock::Type::identifier:
					{
						CodeBlockReverseIterator it(code_blocks);

						if (it.CurrentType() == CodeBlock::Type::identifier_scope) {
							blocks_to_merge = it.Skipped() + 1;

							it.Next();
							if (it.CurrentType() == CodeBlock::Type::identifier) {
								blocks_to_merge = it.Skipped() + 2;
							}
						}
					}

					break;

				case CodeBlock::Type::access_modifier:
					{
						CodeBlockReverseIterator it(code_blocks);

						if (it.CurrentType() == CodeBlock::Type::identifier) {
							blocks_to_merge = it.Skipped() + 1;
						}
					}
					break;

				case CodeBlock::Type::initialization_list:
					{
						CodeBlockReverseIterator it(code_blocks);

						if (it.CurrentType() == CodeBlock::Type::initialization_list) {
							blocks_to_merge = it.Skipped() + 1;
						}
					}
					break;

				default:
					break;
			}

			if (blocks_to_merge == 0) {
				return false;
			} else {
				while (blocks_to_merge-- > 1) code_blocks.pop_back();

				code_blocks.back().end = code_block.end;
				code_blocks.back().type = code_block.type;

				return true;
			}
		}

		void InsertCodeBlock(const CodeBlock::Type type, const char * begin) {
			const char * code_to_process;
			size_t line_last_block;

			if (code_blocks.empty()) {
				code_to_process = iterator.Begin();
				line_last_block = 1;
			} else {
				auto MergeWithPrevious = [&] () -> bool {
					if (type == CodeBlock::Type::previous_type) return true;
					if (type == code_blocks.back().type) return true;

					switch (code_blocks.back().type) {
						case CodeBlock::Type::arguments_or_parameters:
							return containers.back().parenthesis > 0;

						case CodeBlock::Type::initialization_list:
							return containers.back().type == Container::Type::initialization_list;

						default:
							break;
					}

					return false;
				};

				if (MergeWithPrevious()) {
					code_blocks.back().end = iterator.Pointer() - 1;
					return;
				}

				code_to_process = code_blocks.back().end + 1;
			}

			if (begin > code_to_process) {
				code_blocks.push_back(CodeBlock(CodeBlock::Type::other, code_to_process, begin - 1));
			}

			if (type == CodeBlock::Type::none) return;

			CodeBlock code_block { type, begin, iterator.Pointer() - 1 };

			if (!MergeCodeBlocks(code_block)) {
				code_blocks.push_back(code_block);
			}
		}

		void ParseWhiteSpaces() {
			while (isspace(iterator.Next()));
		}

		void ParseEscapeSequence() {
			iterator.MoveNext();

			if (!iterator.Match(R"(['"\?\\abfnrtv]|[0-7]{3}|x[0-9A-Fa-f]{2}|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8})")) {
				throw Error("Invalid escape sequence", iterator);
			}
		}

		void ParseCharLiteral() {
			auto c = iterator.Next();

			switch (c) {
				case '\'':
					throw Error("Empty character literal found", iterator);
					break;

				case '\\':
					ParseEscapeSequence();
					break;

				default:
					iterator.MoveNext();
					break;
			}

			if (iterator.Value() != '\'') {
				throw Error("Character literal delimiter is missing", iterator);
			}

			iterator.MoveNext();
		}

		void ParseString() {
			bool is_raw_string = (iterator.PreviousValue() == 'R');

			Iterator iterator_start_string = iterator;

			iterator.MoveNext();

			if (is_raw_string) {
				if (!iterator.Match(R"O(([^()\\\s]{0,16})\(.*?\)\1")O")) {
					throw Error("Invalid raw string", iterator_start_string);
				}
			} else {
				for (;;) {
					if (!iterator.AdvanceUntilCharIsFound("\\\"\n")) {
						throw Error("String does not end", iterator_start_string);
					}

					switch (iterator.Value()) {
						case '"':
							iterator.MoveNext();
							return;

						case '\\':
							ParseEscapeSequence();
							break;

						default:
							throw Error("String does not end", iterator_start_string);
							break;
					}
				}
			}
		}

		void ParseDirective() {
			iterator.MoveNext();

			if (iterator.Match(R"(.*?/\*)")) { // Directive followed by C style comment in the same line
				if (!iterator.Match(R"(.*?\*/.*)")) {
					iterator.MoveBack(2); // Comment does not end on this line, it has to be processed separately
				}
			} else { // Directive
				iterator.AdvanceUntilCharIsFound('\n');
				iterator.MoveNext();
			}
		}

		bool ParseComments() {
			switch (iterator.Peek()) {
				case '*': // C comment
					if (iterator.Match(R"(/\*[\s\S]*?\*/\s*\n*)")) {
						return true;
					} else {
						throw Error("C style comment (/*) does not end (*/)", iterator);
					}
					break;

				case '/': // C++ comment
					bool match = iterator.Match("//.*");
					if (match) while (iterator.Match(R"(\s*//.*)"));

					return match;
			}

			return false;
		}

	public:

		/// Creates a parser for an .cppx file.
		/// \param filename Filename path.
		Parser(const boost::filesystem::path & filename) : code(filename, boost::iostreams::mapped_file::readonly), containers { Container(Container::Type::none) }, iterator(code.const_data()) {
			Container::Type next_container = Container::Type::none;
			std::string last_identifier;
			std::string container_name;

			while (unsigned char c = iterator.Value()) {
				CodeBlock::Type code_block_type = CodeBlock::Type::none;

				const char * begin = iterator.Pointer();

				switch (c) {
					case '\'':
						code_block_type = CodeBlock::Type::char_literal;
						ParseCharLiteral();
						break;

					case '"':
						code_block_type = CodeBlock::Type::string_literal;
						ParseString();
						break;

					case '#':
						code_block_type = CodeBlock::Type::directive;
						ParseDirective();
						break;

					case ';':
						code_block_type = CodeBlock::Type::statement_terminator;
						iterator.MoveNext();
						break;

					case '{':
						code_block_type = CodeBlock::Type::begin_group;
						iterator.MoveNext();

						if (next_container == Parser::Container::Type::none || containers.back().type == Container::Type::initialization_list) {
							containers.back().braces++;
						} else {
							containers.push_back(Container(container_name, next_container, 1));
							next_container = Container::Type::none;
							container_name.clear();
						}

						break;

					case '}':
						if (containers.back().braces-- == 0) {
							throw Error("An extra '}' was found. Perhaps you forgot a '{'", iterator);
						}

						if (containers.back().type == Container::Type::initialization_list) {
							code_block_type = CodeBlock::Type::previous_type;

							if (containers.back().braces == 0 && containers.back().parenthesis == 0) {
								containers.pop_back();
							}
						} else {
							code_block_type = CodeBlock::Type::end_group;

							if (containers.back().braces == 0 && containers.size() > 1) {
								containers.pop_back();
							}
						}

						iterator.MoveNext();
						break;

					case '/':
						if (ParseComments()) code_block_type = CodeBlock::Type::comment;
						break;

					case '(':
						code_block_type = CodeBlock::Type::arguments_or_parameters;

						switch (containers.back().type) {
							case Container::Type::function:
							case Container::Type::initialization_list:
								break;

							default:
								{
									CodeBlockReverseIterator it(code_blocks);

									auto c = it.Current();
									if (c != it.End() && c->type == CodeBlock::Type::identifier) {
										if (containers.back().name == c->ToString()) {
											c->type = CodeBlock::Type::constructor_destructor;
											next_container = Container::Type::constructor_destructor;
										} else {
											c->type = CodeBlock::Type::function_name;
											next_container = Container::Type::function;
										}
										container_name = last_identifier;
									}
								}

								break;
						}

						containers.back().parenthesis++;

						iterator.MoveNext();
						break;

					case ')':
						if (containers.back().parenthesis-- == 0) {
							throw Error("An extra ')' was found. Perhaps you forgot a '('", iterator);
						}

						if (containers.back().type == Container::Type::initialization_list) {
							code_block_type = CodeBlock::Type::previous_type;

							if (containers.back().braces == 0 && containers.back().parenthesis == 0) {
								containers.pop_back();
							}
						} else {
							code_block_type = CodeBlock::Type::arguments_or_parameters;
						}

						iterator.MoveNext();
						break;

					case ',':
						if (containers.back().type != Container::Type::initialization_list) {
							CodeBlockReverseIterator it(code_blocks);

							if (it.CurrentType() == CodeBlock::Type::initialization_list) {
								code_block_type = CodeBlock::Type::initialization_list;
								containers.push_back(Container(Container::Type::initialization_list, 0));
							}
						}

						iterator.MoveNext();
						break;

					case ':':
						if (iterator.Next() == ':') {
							code_block_type = CodeBlock::Type::identifier_scope;
							iterator.MoveNext();
						} else if (next_container == Container::Type::constructor_destructor) {
							code_block_type = CodeBlock::Type::initialization_list;
							containers.push_back(Container(Container::Type::initialization_list, 0));
						} else if (last_identifier == "public" || last_identifier == "protected" || last_identifier == "private") {
							code_block_type = CodeBlock::Type::access_modifier;
						}
						break;

					default:
						{
							std::cmatch match_result;

							if (iterator.Match(R"([_a-zA-Z]\w*)", match_result)) { // identifier
								auto match_string = match_result.str();

								if (match_string == "class") {
									code_block_type = CodeBlock::Type::class_keyword;
									next_container = Container::Type::class_container;
									container_name.clear();
								} else if (match_string == "enum") {
									code_block_type = CodeBlock::Type::enumeration;
									next_container = Container::Type::enumeration;
									container_name.clear();
								} else if (match_string == "namespace") {
									code_block_type = CodeBlock::Type::namespace_keyword;
									next_container = Container::Type::namespace_container;
									container_name.clear();
								} else if (match_string == "struct") {
									code_block_type = CodeBlock::Type::struct_keyword;
									next_container = Container::Type::struct_container;
									container_name.clear();
								} else {
									code_block_type = CodeBlock::Type::identifier;
									last_identifier = match_string;
									if (container_name.empty()) container_name = last_identifier;
								}
							} else if (isspace(c)) {
								code_block_type = CodeBlock::Type::empty;
								ParseWhiteSpaces();
							}
						}
						break;
				}

				if (code_block_type == CodeBlock::Type::none) {
					iterator.MoveNext();
				} else {
					InsertCodeBlock(code_block_type, begin);
				}
			}

			InsertCodeBlock(CodeBlock::Type::none, iterator.Pointer());
		}

		const std::vector<CodeBlock> & CodeBlocks() const {
			return code_blocks;
		}
	};
}

#endif // CPPX_PARSER_H
