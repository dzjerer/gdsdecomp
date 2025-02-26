/*************************************************************************/
/*  bytecode_ed80f45.cpp                                                 */
/*************************************************************************/

#include "core/io/marshalls.h"
#include "core/string/print_string.h"
#include "core/templates/rb_map.h"

#include "bytecode_ed80f45.h"

static constexpr char *func_names[] = {

	"sin",
	"cos",
	"tan",
	"sinh",
	"cosh",
	"tanh",
	"asin",
	"acos",
	"atan",
	"atan2",
	"sqrt",
	"fmod",
	"fposmod",
	"floor",
	"ceil",
	"round",
	"abs",
	"sign",
	"pow",
	"log",
	"exp",
	"is_nan",
	"is_inf",
	"ease",
	"decimals",
	"stepify",
	"lerp",
	"dectime",
	"randomize",
	"randi",
	"randf",
	"rand_range",
	"seed",
	"rand_seed",
	"deg2rad",
	"rad2deg",
	"linear2db",
	"db2linear",
	"max",
	"min",
	"clamp",
	"nearest_po2",
	"weakref",
	"funcref",
	"convert",
	"typeof",
	"type_exists",
	"str",
	"print",
	"printt",
	"prints",
	"printerr",
	"printraw",
	"var2str",
	"str2var",
	"var2bytes",
	"bytes2var",
	"range",
	"load",
	"inst2dict",
	"dict2inst",
	"hash",
	"Color8",
	"ColorN",
	"print_stack",
	"instance_from_id",
};

static constexpr uint64_t FUNC_MAX = sizeof(func_names) / sizeof(func_names[0]);

enum Token {

	TK_EMPTY,
	TK_IDENTIFIER,
	TK_CONSTANT,
	TK_SELF,
	TK_BUILT_IN_TYPE,
	TK_BUILT_IN_FUNC,
	TK_OP_IN,
	TK_OP_EQUAL,
	TK_OP_NOT_EQUAL,
	TK_OP_LESS,
	TK_OP_LESS_EQUAL,
	TK_OP_GREATER,
	TK_OP_GREATER_EQUAL,
	TK_OP_AND,
	TK_OP_OR,
	TK_OP_NOT,
	TK_OP_ADD,
	TK_OP_SUB,
	TK_OP_MUL,
	TK_OP_DIV,
	TK_OP_MOD,
	TK_OP_SHIFT_LEFT,
	TK_OP_SHIFT_RIGHT,
	TK_OP_ASSIGN,
	TK_OP_ASSIGN_ADD,
	TK_OP_ASSIGN_SUB,
	TK_OP_ASSIGN_MUL,
	TK_OP_ASSIGN_DIV,
	TK_OP_ASSIGN_MOD,
	TK_OP_ASSIGN_SHIFT_LEFT,
	TK_OP_ASSIGN_SHIFT_RIGHT,
	TK_OP_ASSIGN_BIT_AND,
	TK_OP_ASSIGN_BIT_OR,
	TK_OP_ASSIGN_BIT_XOR,
	TK_OP_BIT_AND,
	TK_OP_BIT_OR,
	TK_OP_BIT_XOR,
	TK_OP_BIT_INVERT,
	//TK_OP_PLUS_PLUS,
	//TK_OP_MINUS_MINUS,
	TK_CF_IF,
	TK_CF_ELIF,
	TK_CF_ELSE,
	TK_CF_FOR,
	TK_CF_DO,
	TK_CF_WHILE,
	TK_CF_SWITCH,
	TK_CF_CASE,
	TK_CF_BREAK,
	TK_CF_CONTINUE,
	TK_CF_PASS,
	TK_CF_RETURN,
	TK_PR_FUNCTION,
	TK_PR_CLASS,
	TK_PR_EXTENDS,
	TK_PR_ONREADY,
	TK_PR_TOOL,
	TK_PR_STATIC,
	TK_PR_EXPORT,
	TK_PR_SETGET,
	TK_PR_CONST,
	TK_PR_VAR,
	TK_PR_ENUM,
	TK_PR_PRELOAD,
	TK_PR_ASSERT,
	TK_PR_YIELD,
	TK_PR_SIGNAL,
	TK_PR_BREAKPOINT,
	TK_BRACKET_OPEN,
	TK_BRACKET_CLOSE,
	TK_CURLY_BRACKET_OPEN,
	TK_CURLY_BRACKET_CLOSE,
	TK_PARENTHESIS_OPEN,
	TK_PARENTHESIS_CLOSE,
	TK_COMMA,
	TK_SEMICOLON,
	TK_PERIOD,
	TK_QUESTION_MARK,
	TK_COLON,
	TK_NEWLINE,
	TK_CONST_PI,
	TK_ERROR,
	TK_EOF,
	TK_CURSOR, //used for code completion
	TK_MAX
};

Error GDScriptDecomp_ed80f45::decompile_buffer(Vector<uint8_t> p_buffer) {
	//Cleanup
	script_text = String();

	//Load bytecode
	Vector<StringName> identifiers;
	Vector<Variant> constants;
	VMap<uint32_t, uint32_t> lines;
	Vector<uint32_t> tokens;

	const uint8_t *buf = p_buffer.ptr();
	int total_len = p_buffer.size();
	ERR_FAIL_COND_V(p_buffer.size() < 24 || p_buffer[0] != 'G' || p_buffer[1] != 'D' || p_buffer[2] != 'S' || p_buffer[3] != 'C', ERR_INVALID_DATA);

	int version = decode_uint32(&buf[4]);
	if (version != bytecode_version) {
		ERR_FAIL_COND_V(version > bytecode_version, ERR_INVALID_DATA);
	}
	int identifier_count = decode_uint32(&buf[8]);
	int constant_count = decode_uint32(&buf[12]);
	int line_count = decode_uint32(&buf[16]);
	int token_count = decode_uint32(&buf[20]);

	const uint8_t *b = &buf[24];
	total_len -= 24;

	identifiers.resize(identifier_count);
	for (int i = 0; i < identifier_count; i++) {
		int len = decode_uint32(b);
		ERR_FAIL_COND_V(len > total_len, ERR_INVALID_DATA);
		b += 4;
		Vector<uint8_t> cs;
		cs.resize(len);
		for (int j = 0; j < len; j++) {
			cs.write[j] = b[j] ^ 0xb6;
		}

		cs.write[cs.size() - 1] = 0;
		String s;
		s.parse_utf8((const char *)cs.ptr());
		b += len;
		total_len -= len + 4;
		identifiers.write[i] = s;
	}

	constants.resize(constant_count);
	for (int i = 0; i < constant_count; i++) {
		Variant v;
		int len;
		Error err = VariantDecoderCompat::decode_variant_compat(variant_ver_major, v, b, total_len, &len);
		if (err) {
			error_message = RTR("Invalid constant");
			return err;
		}
		b += len;
		total_len -= len;
		constants.write[i] = v;
	}

	ERR_FAIL_COND_V(line_count * 8 > total_len, ERR_INVALID_DATA);

	for (int i = 0; i < line_count; i++) {
		uint32_t token = decode_uint32(b);
		b += 4;
		uint32_t linecol = decode_uint32(b);
		b += 4;

		lines.insert(token, linecol);
		total_len -= 8;
	}

	tokens.resize(token_count);

	for (int i = 0; i < token_count; i++) {
		ERR_FAIL_COND_V(total_len < 1, ERR_INVALID_DATA);

		if ((*b) & TOKEN_BYTE_MASK) { //little endian always
			ERR_FAIL_COND_V(total_len < 4, ERR_INVALID_DATA);

			tokens.write[i] = decode_uint32(b) & ~TOKEN_BYTE_MASK;
			b += 4;
		} else {
			tokens.write[i] = *b;
			b += 1;
			total_len--;
		}
	}

	//Decompile script
	String line;
	int indent = 0;

	Token prev_token = TK_NEWLINE;
	for (int i = 0; i < tokens.size(); i++) {
		switch (Token(tokens[i] & TOKEN_MASK)) {
			case TK_EMPTY: {
				//skip
			} break;
			case TK_IDENTIFIER: {
				uint32_t identifier = tokens[i] >> TOKEN_BITS;
				ERR_FAIL_COND_V(identifier >= (uint32_t)identifiers.size(), ERR_INVALID_DATA);
				line += String(identifiers[identifier]);
			} break;
			case TK_CONSTANT: {
				uint32_t constant = tokens[i] >> TOKEN_BITS;
				ERR_FAIL_COND_V(constant >= (uint32_t)constants.size(), ERR_INVALID_DATA);
				line += get_constant_string(constants, constant);
			} break;
			case TK_SELF: {
				line += "self";
			} break;
			case TK_BUILT_IN_TYPE: {
				line += VariantDecoderCompat::get_variant_type_name(tokens[i] >> TOKEN_BITS, variant_ver_major);
			} break;
			case TK_BUILT_IN_FUNC: {
				ERR_FAIL_COND_V(tokens[i] >> TOKEN_BITS >= FUNC_MAX, ERR_INVALID_DATA);
				line += func_names[tokens[i] >> TOKEN_BITS];
			} break;
			case TK_OP_IN: {
				_ensure_space(line);
				line += "in ";
			} break;
			case TK_OP_EQUAL: {
				_ensure_space(line);
				line += "== ";
			} break;
			case TK_OP_NOT_EQUAL: {
				_ensure_space(line);
				line += "!= ";
			} break;
			case TK_OP_LESS: {
				_ensure_space(line);
				line += "< ";
			} break;
			case TK_OP_LESS_EQUAL: {
				_ensure_space(line);
				line += "<= ";
			} break;
			case TK_OP_GREATER: {
				_ensure_space(line);
				line += "> ";
			} break;
			case TK_OP_GREATER_EQUAL: {
				_ensure_space(line);
				line += ">= ";
			} break;
			case TK_OP_AND: {
				_ensure_space(line);
				line += "and ";
			} break;
			case TK_OP_OR: {
				_ensure_space(line);
				line += "or ";
			} break;
			case TK_OP_NOT: {
				_ensure_space(line);
				line += "not ";
			} break;
			case TK_OP_ADD: {
				_ensure_space(line);
				line += "+ ";
			} break;
			case TK_OP_SUB: {
				if (prev_token != TK_NEWLINE)
					_ensure_space(line);
				line += "- ";
				//TODO: do not add space after unary "-"
			} break;
			case TK_OP_MUL: {
				_ensure_space(line);
				line += "* ";
			} break;
			case TK_OP_DIV: {
				_ensure_space(line);
				line += "/ ";
			} break;
			case TK_OP_MOD: {
				_ensure_space(line);
				line += "% ";
			} break;
			case TK_OP_SHIFT_LEFT: {
				_ensure_space(line);
				line += "<< ";
			} break;
			case TK_OP_SHIFT_RIGHT: {
				_ensure_space(line);
				line += ">> ";
			} break;
			case TK_OP_ASSIGN: {
				_ensure_space(line);
				line += "= ";
			} break;
			case TK_OP_ASSIGN_ADD: {
				_ensure_space(line);
				line += "+= ";
			} break;
			case TK_OP_ASSIGN_SUB: {
				_ensure_space(line);
				line += "-= ";
			} break;
			case TK_OP_ASSIGN_MUL: {
				_ensure_space(line);
				line += "*= ";
			} break;
			case TK_OP_ASSIGN_DIV: {
				_ensure_space(line);
				line += "/= ";
			} break;
			case TK_OP_ASSIGN_MOD: {
				_ensure_space(line);
				line += "%= ";
			} break;
			case TK_OP_ASSIGN_SHIFT_LEFT: {
				_ensure_space(line);
				line += "<<= ";
			} break;
			case TK_OP_ASSIGN_SHIFT_RIGHT: {
				_ensure_space(line);
				line += ">>= ";
			} break;
			case TK_OP_ASSIGN_BIT_AND: {
				_ensure_space(line);
				line += "&= ";
			} break;
			case TK_OP_ASSIGN_BIT_OR: {
				_ensure_space(line);
				line += "|= ";
			} break;
			case TK_OP_ASSIGN_BIT_XOR: {
				_ensure_space(line);
				line += "^= ";
			} break;
			case TK_OP_BIT_AND: {
				_ensure_space(line);
				line += "& ";
			} break;
			case TK_OP_BIT_OR: {
				_ensure_space(line);
				line += "| ";
			} break;
			case TK_OP_BIT_XOR: {
				_ensure_space(line);
				line += "^ ";
			} break;
			case TK_OP_BIT_INVERT: {
				_ensure_space(line);
				line += "~ ";
			} break;
			//case TK_OP_PLUS_PLUS: {
			//	line += "++";
			//} break;
			//case TK_OP_MINUS_MINUS: {
			//	line += "--";
			//} break;
			case TK_CF_IF: {
				if (prev_token != TK_NEWLINE)
					_ensure_space(line);
				line += "if ";
			} break;
			case TK_CF_ELIF: {
				line += "elif ";
			} break;
			case TK_CF_ELSE: {
				if (prev_token != TK_NEWLINE)
					_ensure_space(line);
				line += "else ";
			} break;
			case TK_CF_FOR: {
				line += "for ";
			} break;
			case TK_CF_DO: {
				line += "do ";
			} break;
			case TK_CF_WHILE: {
				line += "while ";
			} break;
			case TK_CF_SWITCH: {
				line += "swith ";
			} break;
			case TK_CF_CASE: {
				line += "case ";
			} break;
			case TK_CF_BREAK: {
				line += "break";
			} break;
			case TK_CF_CONTINUE: {
				line += "continue";
			} break;
			case TK_CF_PASS: {
				line += "pass";
			} break;
			case TK_CF_RETURN: {
				line += "return ";
			} break;
			case TK_PR_FUNCTION: {
				line += "func ";
			} break;
			case TK_PR_CLASS: {
				line += "class ";
			} break;
			case TK_PR_EXTENDS: {
				if (prev_token != TK_NEWLINE)
					_ensure_space(line);
				line += "extends ";
			} break;
			case TK_PR_ONREADY: {
				line += "onready ";
			} break;
			case TK_PR_TOOL: {
				line += "tool ";
			} break;
			case TK_PR_STATIC: {
				line += "static ";
			} break;
			case TK_PR_EXPORT: {
				line += "export ";
			} break;
			case TK_PR_SETGET: {
				line += " setget ";
			} break;
			case TK_PR_CONST: {
				line += "const ";
			} break;
			case TK_PR_VAR: {
				if (line != String() && prev_token != TK_PR_ONREADY)
					line += " ";
				line += "var ";
			} break;
			case TK_PR_ENUM: {
				line += "enum ";
			} break;
			case TK_PR_PRELOAD: {
				line += "preload";
			} break;
			case TK_PR_ASSERT: {
				line += "assert ";
			} break;
			case TK_PR_YIELD: {
				line += "yield ";
			} break;
			case TK_PR_SIGNAL: {
				line += "signal ";
			} break;
			case TK_PR_BREAKPOINT: {
				line += "breakpoint ";
			} break;
			case TK_BRACKET_OPEN: {
				line += "[";
			} break;
			case TK_BRACKET_CLOSE: {
				line += "]";
			} break;
			case TK_CURLY_BRACKET_OPEN: {
				line += "{";
			} break;
			case TK_CURLY_BRACKET_CLOSE: {
				line += "}";
			} break;
			case TK_PARENTHESIS_OPEN: {
				line += "(";
			} break;
			case TK_PARENTHESIS_CLOSE: {
				line += ")";
			} break;
			case TK_COMMA: {
				line += ", ";
			} break;
			case TK_SEMICOLON: {
				line += ";";
			} break;
			case TK_PERIOD: {
				line += ".";
			} break;
			case TK_QUESTION_MARK: {
				line += "?";
			} break;
			case TK_COLON: {
				line += ":";
			} break;
			case TK_NEWLINE: {
				for (int j = 0; j < indent; j++) {
					script_text += "\t";
				}
				script_text += line + "\n";
				line = String();
				indent = tokens[i] >> TOKEN_BITS;
			} break;
			case TK_CONST_PI: {
				line += "PI";
			} break;
			case TK_ERROR: {
				//skip - invalid
			} break;
			case TK_EOF: {
				//skip - invalid
			} break;
			case TK_CURSOR: {
				//skip - invalid
			} break;
			case TK_MAX: {
				//skip - invalid
			} break;
		}
		prev_token = Token(tokens[i] & TOKEN_MASK);
	}

	if (!line.is_empty()) {
		for (int j = 0; j < indent; j++) {
			script_text += "\t";
		}
		script_text += line + "\n";
	}

	if (script_text == String()) {
		error_message = RTR("Invalid token");
		return ERR_INVALID_DATA;
	}

	return OK;
}

namespace {
// we should be after the open parenthesis, which has already been checked
bool test_built_in_func_arg_count(const Vector<uint32_t> &tokens, Pair<int, int> arg_count, int &curr_pos) {
	int pos = curr_pos;
	int comma_count = 0;
	int min_args = arg_count.first;
	int max_args = arg_count.second;
	uint32_t t = tokens[pos] & 255; // TOKEN_MASK for all revisions
	int bracket_open = 0;

	if (min_args == 0 && max_args == 0) {
		for (; pos < tokens.size(); pos++, t = tokens[pos] & 255) {
			if (t == TK_PARENTHESIS_CLOSE) {
				break;
			} else if (t != TK_NEWLINE) {
				return false;
			}
		}
		// if we didn't find a close parenthesis, then we have an error
		if (pos == tokens.size()) {
			return false;
		}
		curr_pos = pos;
		return true;
	}
	// count the commas
	// at least in 3.x and below, the only time commas are allowed in function args are other expressions
	// this is not the case for GDScript 2.0 (4.x), due to lambdas, but that doesn't have a compiled version yet
	for (; pos < tokens.size(); pos++, t = tokens[pos] & 255) {
		switch (t) {
			case TK_BRACKET_OPEN:
			case TK_CURLY_BRACKET_OPEN:
			case TK_PARENTHESIS_OPEN:
				bracket_open++;
				break;
			case TK_BRACKET_CLOSE:
			case TK_CURLY_BRACKET_CLOSE:
			case TK_PARENTHESIS_CLOSE:
				bracket_open--;
				break;
			case TK_COMMA:
				if (bracket_open == 0) {
					comma_count++;
				}
				break;
			default:
				break;
		}
		if (bracket_open == -1) {
			break;
		}
	}
	// trailing commas are not allowed after the last argument
	if (pos == tokens.size() || t != TK_PARENTHESIS_CLOSE || comma_count < min_args - 1 || comma_count > max_args - 1) {
		return false;
	}
	curr_pos = pos;
	return true;
}
} //namespace

// bytecode rev ed80f45 (Godot v2.1.3-v2.1.6) introduced TK_PR_ENUM token, need to test for this
// also test function arg counts (only fail cases)
GDScriptDecomp::BYTECODE_TEST_RESULT GDScriptDecomp_ed80f45::test_bytecode(Vector<uint8_t> buffer) {
	Vector<StringName> identifiers;
	Vector<Variant> constants;
	Vector<uint32_t> tokens;
	Error err = get_ids_consts_tokens(buffer, bytecode_version, identifiers, constants, tokens);
	ERR_FAIL_COND_V_MSG(err != OK, BYTECODE_TEST_RESULT::BYTECODE_TEST_CORRUPT, "Failed to get identifiers, constants, and tokens from bytecode.");
	int token_count = tokens.size();
	bool tested_enum_case = false;
	for (int i = 0; i < token_count; i++) {
		// Test for the existence of the TK_PR_ENUM token
		// the next token after TK_PR_ENUM should be TK_IDENTIFIER OR TK_CURLY_BRACKET_OPEN
		if ((tokens[i] & TOKEN_MASK) == TK_PR_ENUM) { // ignore all tokens until we find TK_PR_ENUM
			tested_enum_case = true;
			if (i + 1 >= token_count) { // if we're at the end of the token list, then we don't have a valid enum token OR a valud TK_PR_PRELOAD token
				return BYTECODE_TEST_RESULT::BYTECODE_TEST_FAIL;
			}

			// ENUM requires TK_IDENTIFIER or TK_CURLY_BRACKET_OPEN as the next token
			if ((tokens[i + 1] & TOKEN_MASK) != TK_IDENTIFIER && (tokens[i + 1] & TOKEN_MASK) != TK_CURLY_BRACKET_OPEN) {
				// If we don't have that, this is actually TK_PR_PRELOAD of the previous bytecode version, which requires TK_PARENTHESIS_OPEN as the next token
				if ((tokens[i + 1] & TOKEN_MASK) == TK_CURLY_BRACKET_CLOSE) { // TK_CURLY_BRACKET_CLOSE in this rev is TK_PARENTHESIS_OPEN in the previous rev
					// TODO: Something special here?
					return BYTECODE_TEST_RESULT::BYTECODE_TEST_FAIL;
				}
				// if we don't have TK_PARENTHESIS_OPEN, then this is likely another bytecode revision or something screwy is going on, return BYTECODE_TEST_CORRUPT
				return BYTECODE_TEST_RESULT::BYTECODE_TEST_CORRUPT;
			}
			// keep testing until the end
		} else if ((tokens[i] & TOKEN_MASK) == TK_BUILT_IN_FUNC) {
			int func_id = tokens[i] >> TOKEN_BITS;

			// this rev was the highest possible bytecode version 10, there were no more builtin funcs after this;
			// if the func_id is >= size of func_names, then this is corrupt
			if (func_id >= FUNC_MAX) {
				return BYTECODE_TEST_RESULT::BYTECODE_TEST_CORRUPT;
			}
			// All bytecode 10 revisions had the same position for TK_BUILT_IN_FUNC; if this check fails, then the bytecode is corrupt
			if (i + 2 >= token_count) {
				return BYTECODE_TEST_RESULT::BYTECODE_TEST_CORRUPT;
			}
			i++;
			if (tokens[i] != TK_PARENTHESIS_OPEN) {
				// ENUM shifted the position of TK_PARENTHESIS_OPEN by 1, so if we don't have TK_PARTEHESIS_OPEN, then this is another version 10 revision
				return BYTECODE_TEST_RESULT::BYTECODE_TEST_FAIL;
			}
			i++;

			auto arg_count = get_arg_count_for_builtin(func_names[func_id]);
			if (!test_built_in_func_arg_count(tokens, arg_count, i)) {
				return BYTECODE_TEST_RESULT::BYTECODE_TEST_FAIL;
			};
			// we keep going until the end for func arg count checking
		}
	}
	// we didn't find an enum token
	return BYTECODE_TEST_RESULT::BYTECODE_TEST_UNKNOWN;
}
