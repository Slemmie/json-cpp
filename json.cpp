#include "json.h"

#include <sstream>
#include <functional>
#include <charconv>

JSON& JSON::operator [] (std::string_view key) {
	if (std::holds_alternative <std::monostate> (this->m_data)) {
		this->m_data = object_type { };
	}
	if (!std::holds_alternative <object_type> (this->m_data)) {
		throw std::runtime_error("JSON& JSON::operator [] (key), but value is not object type");
	}
	return std::get <object_type> (this->m_data) [std::string(key)];
}

const JSON& JSON::operator [] (std::string_view key) const {
	if (!std::holds_alternative <object_type> (this->m_data)) {
		throw std::runtime_error("const JSON& JSON::operator [] (key), but value is not object type");
	}
	const object_type& obj = std::get <object_type> (this->m_data);
	if (auto it = obj.find(std::string(key)); it != obj.end()) {
		return it->second;
	}
	throw std::invalid_argument("JSON object does not contain the queried key");
}

JSON& JSON::operator [] (size_t index) {
	if (std::holds_alternative <std::monostate> (this->m_data)) {
		this->m_data = array_type { };
	}
	if (!std::holds_alternative <array_type> (this->m_data)) {
		throw std::runtime_error("JSON& JSON::operator [] (index), but value is not array type");
	}
	auto& arr = std::get <array_type> (this->m_data);
	if (index >= arr.size()) {
		throw std::invalid_argument("out of bounds on JSON::array_type");
	}
	return arr[index];
}

const JSON& JSON::operator [] (size_t index) const {
	if (!std::holds_alternative <array_type> (this->m_data)) {
		throw std::runtime_error("const JSON& JSON::operator [] (index), but value is not array type");
	}
	const auto& arr = std::get <array_type> (this->m_data);
	if (index >= arr.size()) {
		throw std::invalid_argument("out of bounds on JSON::array_type");
	}
	return arr[index];
}

JSON JSON::parse(std::string_view data) {
	auto consume_string = [&] (std::istringstream& iss, std::string_view str) -> bool {
		for (char c : str) {
			if (iss.peek() != c) {
				return false;
			}
			(void) iss.get();
		}
		return true;
	};
	std::function <JSON (std::istringstream&)> parse_literal, parse_string, parse_array, parse_object, parse_number, parse_value;
	parse_literal = [&] (std::istringstream& iss) -> JSON {
		iss >> std::ws;
		if (iss.peek() == 'n' && consume_string(iss, "null")) {
			return JSON(nullptr);
		}
		if (iss.peek() == 't' && consume_string(iss, "true")) {
			return JSON(true);
		}
		if (iss.peek() == 'f' && consume_string(iss, "false")) {
			return JSON(false);
		}
		throw std::invalid_argument("failed to parse JSON data: invalid literal");
	};
	parse_string = [&] (std::istringstream& iss) -> JSON {
		iss >> std::ws;
		if (iss.peek() != '"') {
			throw std::invalid_argument("failed to parse JSON data: expected '\"' character");
		}
		(void) iss.get();
		std::string result;
		while (iss.peek() != std::char_traits <char>::eof() && iss.peek() != '"') {
			char c = iss.get();
			if (c == '\\') {
				if (iss.peek() == std::char_traits <char>::eof()) {
					throw std::invalid_argument("failed to parse JSON data: unfinished escape sequence");
				}
				char esc = iss.get();
				switch (esc) {
					case 'b': result += '\b'; break;
					case 'f': result += '\f'; break;
					case 'n': result += '\n'; break;
					case 'r': result += '\r'; break;
					case 't': result += '\t'; break;
					case '"':
					case '\\':
					case '/':
						result += esc;
						break;
				}
			} else {
				result += c;
			}
		}
		if (iss.peek() != '"') {
			throw std::invalid_argument("failed to parse JSON data: expected '\"' character");
		}
		(void) iss.get();
		return JSON(result);
	};
	parse_array = [&] (std::istringstream& iss) -> JSON {
		iss >> std::ws;
		if (iss.peek() != '[') {
			throw std::invalid_argument("failed to parse JSON data: expected '[' character");
		}
		(void) iss.get();
		iss >> std::ws;
		array_type arr;
		if (iss.peek() == ']') {
			(void) iss.get();
			return JSON(arr);
		}
		arr.push_back(parse_value(iss));
		iss >> std::ws;
		while (iss.peek() == ',') {
			(void) iss.get();
			arr.push_back(parse_value(iss));
			iss >> std::ws;
		}
		iss >> std::ws;
		if (iss.peek() != ']') {
			throw std::invalid_argument("failed to parse JSON data: expected ']' character");
		}
		(void) iss.get();
		return JSON(arr);
	};
	parse_object = [&] (std::istringstream& iss) -> JSON {
		iss >> std::ws;
		if (iss.peek() != '{') {
			throw std::invalid_argument("failed to parse JSON data: expected '{' character");
		}
		(void) iss.get();
		iss >> std::ws;
		object_type obj;
		if (iss.peek() == '}') {
			(void) iss.get();
			return JSON(obj);
		}
		while (true) {
			if (iss.peek() != '"') {
				throw std::invalid_argument("failed to parse JSON data: expected '\"' character");
			}
			JSON key_val = parse_string(iss);
			iss >> std::ws;
			if (iss.peek() != ':') {
				throw std::invalid_argument("failed to parse JSON data: expected ':' character");
			}
			(void) iss.get();
			iss >> std::ws;
			obj[key_val.as_string()] = parse_value(iss);
			iss >> std::ws;
			if (iss.peek() != ',') {
				break;
			}
			(void) iss.get();
			iss >> std::ws;
		}
		iss >> std::ws;
		if (iss.peek() != '}') {
			throw std::invalid_argument("failed to parse JSON data: expected '}' character");
		}
		(void) iss.get();
		return JSON(obj);
	};
	parse_number = [&] (std::istringstream& iss) -> JSON {
		iss >> std::ws;
		if (iss.peek() != '-' && iss.peek() != '+' && !std::isdigit(static_cast <unsigned char> (iss.peek()))) {
			throw std::invalid_argument("failed to parse JSON data: expected number");
		}
		std::string str_num;
		if (iss.peek() == '-' || iss.peek() == '+') {
			str_num += iss.get();
		}
		if (iss.peek() == std::char_traits <char>::eof()) {
			throw std::invalid_argument("failed to parse JSON data: unexpected EOF");
		}
		if (!std::isdigit(static_cast <unsigned char> (iss.peek()))) {
			throw std::invalid_argument("failed to parse JSON data: expected digit");
		}
		while (std::isdigit(static_cast <unsigned char> (iss.peek()))) {
			str_num += iss.get();
		}
		bool is_integer = true;
		if (iss.peek() == '.') {
			is_integer = false;
			str_num += iss.get();
			if (!std::isdigit(static_cast <unsigned char> (iss.peek()))) {
				throw std::invalid_argument("failed to parse JSON data: expected digit");
			}
			while (std::isdigit(static_cast <unsigned char> (iss.peek()))) {
				str_num += iss.get();
			}
		}
		if (iss.peek() == 'e' || iss.peek() == 'E') {
			is_integer = false;
			str_num += iss.get();
			if (iss.peek() == '+' || iss.peek() == '-') {
				str_num += iss.get();
			}
			if (!std::isdigit(static_cast <unsigned char> (iss.peek()))) {
				throw std::invalid_argument("failed to parse JSON data: expected digit");
			}
			while (std::isdigit(static_cast <unsigned char> (iss.peek()))) {
				str_num += iss.get();
			}
		}
		if (is_integer) {
			int64_t value;
			auto attempt_result = std::from_chars(str_num.data(), str_num.data() + str_num.size(), value);
			if (attempt_result.ec != std::errc()) {
				return JSON(std::stold(str_num));
			}
			return JSON(value);
		}
		return JSON(std::stold(str_num));
	};
	parse_value = [&] (std::istringstream& iss) -> JSON {
		iss >> std::ws;
		if (iss.peek() == std::char_traits <char>::eof()) {
			throw std::invalid_argument("failed to parse JSON data: unexpected EOF");
		}
		switch (iss.peek()) {
			case 'n': // null
			case 't': // true
			case 'f': // false
				return parse_literal(iss);
			case '"':
				return parse_string(iss);
			case '[':
				return parse_array(iss);
			case '{':
				return parse_object(iss);
			default:
				if (iss.peek() == '-' || iss.peek() != '+' || std::isdigit(static_cast <unsigned char> (iss.peek()))) {
					return parse_number(iss);
				}
				break;
		}
		throw std::invalid_argument("failed to parse JSON: unexpected character");
	};
	std::istringstream iss { std::string(data) };
	JSON result = parse_value(iss);
	iss >> std::ws;
	if (iss.peek() != std::char_traits <char>::eof()) {
		throw std::invalid_argument("failed to parse JSON data: residual data");
	}
	return result;
}

std::string JSON::to_string(bool compact) const {
	return this->m_to_string(compact, 0);
}

std::string JSON::m_to_string(bool compact, size_t tab_cnt) const {
	auto if_ncompact = [compact] (std::string_view str) -> std::string {
		return compact ? "" : std::string(str);
	};
	std::string result;
	switch (this->type()) {
		case Type::Null: result = "null"; break;
		case Type::Bool: result = this->as_bool() ? "true" : "false"; break;
		case Type::Real: result = std::to_string(this->as_real()); break;
		case Type::Integer: result = std::to_string(this->as_integer()); break;
		case Type::String: result = '"' + this->as_string() + '"'; break;
		case Type::Array: {
			result += "[";
			for (const JSON& val : this->as_array()) {
				result += if_ncompact("\n" + std::string(tab_cnt + 1, '\t')) + val.m_to_string(compact, tab_cnt + 1) + ',';
			}
			if (result.back() == ',') {
				result.pop_back();
			}
			result += if_ncompact("\n" + std::string(tab_cnt, '\t')) + "]";
			break;
		}
		case Type::Object: {
			result += "{";
			for (const auto& [key, val] : this->as_object()) {
				result += if_ncompact("\n" + std::string(tab_cnt + 1, '\t')) + '"' + key + "\":" + if_ncompact(" ") + val.m_to_string(compact, tab_cnt + 1) + ',';
			}
			if (result.back() == ',') {
				result.pop_back();
			}
			result += if_ncompact("\n" + std::string(tab_cnt, '\t')) + "}";
			break;
		}
		default: throw std::runtime_error("failed to format JSON object: unexpected value type");
	}
	return result;
}
