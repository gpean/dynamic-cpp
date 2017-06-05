/*
    Copyright (C) 2009, 2011 Ferruccio Barletta (ferruccio.barletta@gmail.com)

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cassert>
#include <cctype>
#include <iostream>
#include <iomanip>

#include <dynamic/exception.hpp>
#include <dynamic/var.hpp>

namespace dynamic {

const var none;

bool var::less_var::operator () (const var& lhs, const var& rhs) const {
    // if the two vars are of different types, order by type
    code lht = lhs.type(), rht = rhs.type();
    if (lht != rht) return lht < rht;

    // they are of the same type, order by value
    switch (lht) {
    case type_null : return false;
    case type_bool : return boost::get<bool_t>(lhs._var) < boost::get<bool_t>(rhs._var);
    case type_int : return boost::get<int_t>(lhs._var) < boost::get<int_t>(rhs._var);
    case type_double : return boost::get<double_t>(lhs._var) < boost::get<double_t>(rhs._var);
    case type_string : return *(boost::get<string_t>(lhs._var).ps) < *(boost::get<string_t>(rhs._var).ps);
    case type_wstring : return *(boost::get<wstring_t>(lhs._var).ps) < *(boost::get<wstring_t>(rhs._var).ps);
    case type_vector :
    case type_map :
        return false;
    default : throw exception("unhandled type");
    }
}

///
/// append a bool to a collection
///
var& var::operator () (bool n) { return operator() (var(n)); }

///
/// append an int to a collection
///
var& var::operator () (int n) { return operator() (var(n)); }

///
/// append a double to a collection
///
var& var::operator () (double n) { return operator() (var(n)); }

///
/// append a string to a collection
///
var& var::operator () (const std::string& s) { return operator() (var(s)); }

///
/// append a string constant to a collection
///
var& var::operator () (const char* s) { return operator() (var(s)); }

///
/// append a wide string to a collection
///
var& var::operator () (const std::wstring& s) { return operator() (var(s)); }

///
/// append a wide string constant to a collection
///
var& var::operator () (const wchar_t* s) { return operator() (var(s)); }

///
/// append a single value to a collection
///
struct var::append_value_visitor : public boost::static_visitor<var&>
{
    append_value_visitor(var& self, const var& value) : self(self), value(value) {}
    var& self;
    const var& value;
    result_type operator () (const null_t&) const { throw exception("invalid () operation on none"); }
    result_type operator () (const bool_t&) const { throw exception("invalid () operation on bool"); }
    result_type operator () (const int_t&) const { throw exception("invalid () operation on int"); }
    result_type operator () (const double_t&) const { throw exception("invalid () operation on double"); }
    result_type operator () (const string_t& value) const { throw exception("invalid () operation on string"); }
    result_type operator () (const wstring_t& value) const { throw exception("invalid () operation on wstring"); }
    result_type operator () (const vector_ptr& ptr) const { ptr->push_back(value); return self; }
    result_type operator () (const map_ptr& ptr) const { ptr->insert(map_type::value_type(value, none)); return self; }
};
var& var::operator () (const var& v) {    
    return boost::apply_visitor(append_value_visitor(*this, v), _var);
}

///
/// add a key,value to a map
///
struct var::append_key_value_visitor : public boost::static_visitor<var&>
{
    append_key_value_visitor(var& self, const var& key, const var& value) : self(self), key(key), value(value) {}
    var& self;
    const var& key;
    const var& value;
    result_type operator () (const null_t&) const { throw exception("invalid (,) operation on none"); }
    result_type operator () (const bool_t&) const { throw exception("invalid (,) operation on bool"); }
    result_type operator () (const int_t&) const { throw exception("invalid (,) operation on int"); }
    result_type operator () (const double_t&) const { throw exception("invalid (,) operation on double"); }
    result_type operator () (const string_t& value) const { throw exception("invalid (,) operation on string"); }
    result_type operator () (const wstring_t& value) const { throw exception("invalid (,) operation on wstring"); }
    result_type operator () (const vector_ptr& ptr) const { throw exception("invalid (,) operation on vector"); }
    result_type operator () (const map_ptr& ptr) const { ptr->insert(map_type::value_type(key, value)); return self; }
};
var& var::operator () (const var& key, const var& value) {
    return boost::apply_visitor(append_key_value_visitor(*this, key, value), _var);
}

///
/// count of objects in a collection or characters in a string
///
struct var::count_visitor : public boost::static_visitor<size_type>
{
    result_type operator () (const null_t&) const { throw exception("invalid .count() operator on none"); }
    result_type operator () (const bool_t&) const { throw exception("invalid .count() operator on bool"); }
    result_type operator () (const int_t&) const { throw exception("invalid .count() operator on int"); }
    result_type operator () (const double_t&) const { throw exception("invalid .count() operator on double"); }
    result_type operator () (const string_t& value) const { return value.ps->length(); }
    result_type operator () (const wstring_t& value) const { return value.ps->length(); }
    result_type operator () (const vector_ptr& ptr) const { return static_cast<result_type>(ptr->size()); }
    result_type operator () (const map_ptr& ptr) const { return static_cast<result_type>(ptr->size()); }
};
var::size_type var::count() const {
    return boost::apply_visitor(count_visitor(), _var);
}

///
/// index[] operator for collections
///
struct var::index_int_visitor : public boost::static_visitor<var&>
{
    index_int_visitor(int n) : n(n) {}
    int n;
    result_type operator () (const null_t&) const { throw exception("cannot apply [int] to none"); }
    result_type operator () (const bool_t&) const { throw exception("cannot apply [int] to bool"); }
    result_type operator () (const int_t&) const { throw exception("cannot apply [int] to int"); }
    result_type operator () (const double_t&) const { throw exception("cannot apply [int] to double"); }
    result_type operator () (const string_t&) const { throw exception("cannot apply [int] to string"); }
    result_type operator () (const wstring_t&) const { throw exception("cannot apply [int] to wstring"); }
    result_type operator () (const vector_ptr& ptr) const
    {
        if (n < 0 || n >= int(ptr->size()))
            throw exception("[int] out of range in vector");
        return (*ptr)[n];
    }
    result_type operator () (const map_ptr& ptr) const
    {
        var key(n);
        map_type::iterator it = ptr->find(key);
        if (it == ptr->end())
            throw exception("[int] not found in map");
        return it->second;
    }
};
var& var::operator [] (int n) {
    return boost::apply_visitor(index_int_visitor(n), _var);
}

///
/// index a collection with a double
///
var& var::operator [] (double n) { return operator[] (var(n)); }

///
/// index a collection with a string
///
var& var::operator [] (const std::string& s) { return operator[] (var(s)); }

///
/// index a collection with a string constant
///
var& var::operator [] (const char* s) { return operator[] (var(s)); }

///
/// index a collection with a wide string
///
var& var::operator [] (const std::wstring& s) { return operator[] (var(s)); }

///
/// index a collection with a wide string constant
///
var& var::operator [] (const wchar_t* s) { return operator[] (var(s)); }
    
///
/// index a collection
///
struct var::index_var_visitor : public boost::static_visitor<var&>
{
    index_var_visitor(const var& key) : key(key) {}
    const var& key;
    result_type operator () (const null_t&) const { throw exception("cannot apply [var] to none"); }
    result_type operator () (const bool_t&) const { throw exception("cannot apply [var] to bool"); }
    result_type operator () (const int_t&) const { throw exception("cannot apply [var] to int"); }
    result_type operator () (const double_t&) const { throw exception("cannot apply [var] to double"); }
    result_type operator () (const string_t&) const { throw exception("cannot apply [var] to string"); }
    result_type operator () (const wstring_t&) const { throw exception("cannot apply [var] to wstring"); }
    result_type operator () (const vector_ptr& ptr) const { throw exception("vector[] requires int"); }
    result_type operator () (const map_ptr& ptr) const
    {
        // See Effective STL (Meyers) item 45
        map_type::iterator it = ptr->lower_bound(key);
        if ((it == ptr->end()) || (ptr->key_comp()(key, it->first)))
        {
            it = ptr->insert(it, map_type::value_type(key, none));
        }
        return it->second;
    }
};
var& var::operator [] (const var& v) {
    return boost::apply_visitor(index_var_visitor(v), _var);
}

const var& var::operator [] (const var& v) const {
    return boost::apply_visitor(index_var_visitor(v), _var);
}

///
/// write a var to an ostream
///
std::ostream& var::_write_var(std::ostream& os) const {
    switch (type()) {
    case type_null :    os << "null"; return os;
    case type_bool:     os << (boost::get<bool_t>(_var) ? "true" : "false"); return os;
    case type_int :     os << boost::get<int_t>(_var); return os;
    case type_double :  os << boost::get<double_t>(_var); return os;
    case type_string :  return _write_string(os);
    case type_wstring : return _write_wstring(os);
    case type_vector :
    case type_map :     return _write_collection(os);
    default :           throw exception("var::_write_var(ostream) unhandled type");
    }
}

///
/// write a string to an ostream
///
std::ostream& var::_write_string(std::ostream& os) const {
    assert(is_string());
    os << '"';
    for (const char* s = (*boost::get<string_t>(_var).ps).c_str(); *s; ++s)
        switch (*s) {
        case '\b' : os << "\\b"; break;
        case '\r' : os << "\\r"; break;
        case '\n' : os << "\\n"; break;
        case '\f' : os << "\\f"; break;
        case '\t' : os << "\\t"; break;
        case '\\' : os << "\\\\"; break;
        case '\"' : os << "\\\""; break;
        case '/' : os << "\\/"; break;
        default :
            if (std::iscntrl(*s)) os << "0" << std::oct << std::setw(3) << std::setfill('0') << int(*s);
            else os << *s;
        }
    os << '"';
    return os;
}

///
/// write a wide string to an ostream
///
std::ostream& var::_write_wstring(std::ostream& os) const {
    assert(is_wstring());
    os << '"';
    for (const wchar_t* s = (*boost::get<wstring_t>(_var).ps).c_str(); *s; ++s)
        switch (*s) {
        case L'\b' : os << L"\\b"; break;
        case L'\r' : os << L"\\r"; break;
        case L'\n' : os << L"\\n"; break;
        case L'\f' : os << L"\\f"; break;
        case L'\t' : os << L"\\t"; break;
        case L'\\' : os << L"\\\\"; break;
        case L'\"' : os << L"\\\""; break;
        case L'/' : os << L"\\/"; break;
        default :
            if (std::iswcntrl(*s)) os << L"0" << std::oct << std::setw(3) << std::setfill('0') << int(*s);
            else os << *s;
        }
    os << '"';
    return os;
}

///
/// write a collection to an ostream
///
std::ostream& var::_write_collection(std::ostream& os) const {
    assert(is_collection());
    const code current = type();
    switch (current)
    {
    case type_vector : os << "[ "; break;
    case type_map : os << "{ "; break;
    default : assert(false);
    }
    for (var::const_iterator vi = begin(); vi != end(); ++vi) {
        switch (current)
        {
        case type_vector:
            if (vi != begin()) os << ", ";
            (*vi)._write_var(os);
            break;
        case type_map:
            if (vi != begin()) os << ", ";
            (*vi)._write_var(os);
            os << " : ";
            (*this)[*vi]._write_var(os);
            break;
        default:
            assert(false);
        }
    }
    switch (current)
    {
    case type_vector : os << " ]"; break;
    case type_map : os << " }"; break;
    default : assert(false);
    }
    return os;
}

///
/// write a var to a wostream
///
std::wostream& var::_write_var(std::wostream& os) const {
    switch (type()) {
    case type_null :    os << "null"; return os;
    case type_bool:     os << (boost::get<bool_t>(_var) ? "true" : "false"); return os;
    case type_int :     os << boost::get<int_t>(_var); return os;
    case type_double :  os << boost::get<double_t>(_var); return os;
    case type_string :  return _write_string(os);
    case type_wstring : return _write_wstring(os);
    case type_vector :
    case type_map :     return _write_collection(os);
    default :           throw exception("var::_write_var(wostream) unhandled type");
    }
}

///
/// write a string to a wostream
///
std::wostream& var::_write_string(std::wostream& os) const {
    assert(is_string());
    os << '\'';
    for (const char* s = (*boost::get<string_t>(_var).ps).c_str(); *s; ++s)
        switch (*s) {
        case '\b' : os << "\\b"; break;
        case '\r' : os << "\\r"; break;
        case '\n' : os << "\\n"; break;
        case '\f' : os << "\\f"; break;
        case '\t' : os << "\\t"; break;
        case '\\' : os << "\\\\"; break;
        case '\'' : os << "\\'"; break;
        default :
            if (*s < ' ') os << "0" << std::oct << std::setw(3) << std::setfill(L'0') << int(*s);
            else os << *s;
        }
    os << '\'';
    return os;
}

///
/// write a wide string to a wostream
///
std::wostream& var::_write_wstring(std::wostream& os) const {
    assert(is_wstring());
    os << '\'';
    for (const wchar_t* s = (*boost::get<wstring_t>(_var).ps).c_str(); *s; ++s)
        switch (*s) {
        case '\b' : os << L"\\b"; break;
        case '\r' : os << L"\\r"; break;
        case '\n' : os << L"\\n"; break;
        case '\f' : os << L"\\f"; break;
        case '\t' : os << L"\\t"; break;
        case '\\' : os << L"\\\\"; break;
        case '\'' : os << L"\\'"; break;
        default :
            if (*s < ' ') os << "0" << std::oct << std::setw(3) << std::setfill(L'0') << int(*s);
            else os << *s;
        }
    os << '\'';
    return os;
}
    
///
/// write a collection to a wostream
///
std::wostream& var::_write_collection(std::wostream& os) const {
    assert(is_collection());
    const code current = type();
    switch (current)
    {
    case type_vector : os << L"[ "; break;
    case type_map : os << L"{ "; break;
    default : assert(false);
    }
    for (var::const_iterator vi = begin(); vi != end(); ++vi) {
        switch (current)
        {
        case type_vector:
            if (vi != begin()) os << L", ";
            (*vi)._write_var(os);
            break;
        case type_map:
            if (vi != begin()) os << L", ";
            (*vi)._write_var(os);
            os << L" : ";
            (*this)[*vi]._write_var(os);
            break;
        default:
            assert(false);
        }
    }
    switch (current)
    {
    case type_vector : os << L" ]"; break;
    case type_map : os << L" }"; break;
    default : assert(false);
    }
    return os;
}

}
