#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <variant>
#include <stdexcept>

class JSON {

public:

	enum class Type {
		Null,
		Bool,
		Real,
		Integer,
		String,
		Array,
		Object
	};

	using array_type = std::vector <JSON>;
	using object_type = std::unordered_map <std::string, JSON>;

	constexpr JSON()                         : m_data(std::monostate { }) { }
	constexpr JSON(std::nullptr_t)           : m_data(std::monostate { }) { }
	constexpr JSON(bool value)               : m_data(value)              { }
	constexpr JSON(long double value)        : m_data(value)              { }
	constexpr JSON(int64_t value)            : m_data(value)              { }
	constexpr JSON(std::string_view value)   : m_data(std::string(value)) { }
	constexpr JSON(const array_type& value)  : m_data(value)              { }
	constexpr JSON(const object_type& value) : m_data(value)              { }

	constexpr Type type() const {
		if (std::holds_alternative <std::monostate> (this->m_data)) return Type::Null;
		if (std::holds_alternative <bool>           (this->m_data)) return Type::Bool;
		if (std::holds_alternative <long double>    (this->m_data)) return Type::Real;
		if (std::holds_alternative <int64_t>        (this->m_data)) return Type::Integer;
		if (std::holds_alternative <std::string>    (this->m_data)) return Type::String;
		if (std::holds_alternative <array_type>     (this->m_data)) return Type::Array;
		if (std::holds_alternative <object_type>    (this->m_data)) return Type::Object;
		throw std::runtime_error("unkown type in JSON::type()");
	}

	constexpr std::string type_str() const {
		switch (this->type()) {
			case Type::Null: return "null";
			case Type::Bool: return "boolean";
			case Type::Real: return "real";
			case Type::Integer: return "integer";
			case Type::String: return "string";
			case Type::Array: return "array";
			case Type::Object: return "object";
			default: break;
		}
		throw std::runtime_error("unkown type in JSON::type()");
	};

	constexpr bool&              as_bool   ()       { return std::get <bool>        (this->m_data); }
	constexpr const bool&        as_bool   () const { return std::get <bool>        (this->m_data); }
	constexpr long double&       as_real   ()       { return std::get <long double> (this->m_data); }
	constexpr const long double& as_real   () const { return std::get <long double> (this->m_data); }
	constexpr int64_t&           as_integer()       { return std::get <int64_t>     (this->m_data); }
	constexpr const int64_t&     as_integer() const { return std::get <int64_t>     (this->m_data); }
	constexpr std::string&       as_string ()       { return std::get <std::string> (this->m_data); }
	constexpr const std::string& as_string () const { return std::get <std::string> (this->m_data); }
	constexpr array_type&        as_array  ()       { return std::get <array_type>  (this->m_data); }
	constexpr const array_type&  as_array  () const { return std::get <array_type>  (this->m_data); }
	constexpr object_type&       as_object ()       { return std::get <object_type> (this->m_data); }
	constexpr const object_type& as_object () const { return std::get <object_type> (this->m_data); }

	constexpr long double as_number() const {
		return this->type() == Type::Integer ? this->as_integer() : this->as_real();
	}

	JSON& operator [] (std::string_view key);
	const JSON& operator [] (std::string_view key) const;

	JSON& operator [] (size_t index);
	const JSON& operator [] (size_t index) const;

	static JSON parse(std::string_view data);

	std::string to_string(bool compact = false) const;

private:

	std::string m_to_string(bool compact, size_t tab_cnt) const;

private:

	std::variant <std::monostate, bool, long double, int64_t, std::string, array_type, object_type> m_data;

};
