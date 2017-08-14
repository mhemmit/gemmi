// Copyright 2017 Global Phasing Ltd.
//
// Reading CIF-JSON (COMCIFS) and mmJSON (PDBj) formats into cif::Document.
// Not finished yet!

#ifndef GEMMI_JSON_HPP_
#define GEMMI_JSON_HPP_
#include <cassert>
#include <cstdio>     // for FILE
#include <string>

#define SAJSON_NO_SORT
#define SAJSON_NUMBERS_AS_STRINGS
#include <sajson.h>

#include "cifdoc.hpp" // for Document, etc
#include "util.hpp"   // for fail, file_open

namespace gemmi {
namespace cif {
using std::size_t;

std::string json_type_as_string(sajson::type t) {
  switch (t) {
    case sajson::TYPE_INTEGER: return "<integer>";
    case sajson::TYPE_DOUBLE:  return "<double>";
    case sajson::TYPE_NULL:    return "<null>";
    case sajson::TYPE_FALSE:   return "<false>";
    case sajson::TYPE_TRUE:    return "<true>";
    case sajson::TYPE_STRING:  return "<string>";
    case sajson::TYPE_ARRAY:   return "<array>";
    case sajson::TYPE_OBJECT:  return "<object>";
    default:           return "<unknown type>";
  }
}

std::string as_cif_value(const sajson::value& val) {
  switch (val.get_type()) {
    case sajson::TYPE_DOUBLE:
      return val.as_string();
    case sajson::TYPE_NULL:
      return "?";
    case sajson::TYPE_FALSE:
      return ".";
    case sajson::TYPE_STRING:
      return quote(val.as_string());
    default:
      fail("Unexpected " + json_type_as_string(val.get_type()) + " in JSON.");
      return "";
  }
}

inline void fill_document_from_sajson(Document& d, const sajson::document& s) {
  // assuming mmJSON here, we'll add handling of CIF-JSON later on
  sajson::value root = s.get_root();
  if (root.get_type() != sajson::TYPE_OBJECT || root.get_length() != 1)
    fail("not mmJSON");
  std::string block_name = root.get_object_key(0).as_string();
  if (!gemmi::starts_with(block_name, "data_"))
    fail("top level key should start with data_");
  d.blocks.emplace_back(block_name.substr(5));
  std::vector<Item>& items = d.blocks[0].items;
  sajson::value top = root.get_object_value(0);
  if (top.get_type() != sajson::TYPE_OBJECT)
    fail("");
  for (size_t i = 0; i != top.get_length(); ++i) {
    std::string category_name = "_" + top.get_object_key(i).as_string() + ".";
    sajson::value category = top.get_object_value(i);
    if (category.get_type() != sajson::TYPE_OBJECT ||
        category.get_length() == 0 ||
        category.get_object_value(0).get_type() != sajson::TYPE_ARRAY)
      fail("");
    size_t cif_cols = category.get_length();
    size_t cif_rows = category.get_object_value(0).get_length();
    if (cif_rows > 1) {
      items.emplace_back(LoopArg{});
      Loop& loop = items.back().loop;
      loop.tags.reserve(cif_cols);
      loop.values.resize(cif_cols * cif_rows);
    }
    for (size_t j = 0; j != cif_cols; ++j) {
      std::string tag = category_name + category.get_object_key(j).as_string();
      sajson::value arr = category.get_object_value(j);
      if (arr.get_type() != sajson::TYPE_ARRAY)
        fail("Expected array, got " + json_type_as_string(arr.get_type()));
      if (arr.get_length() != cif_rows)
        fail("Expected array of length " + std::to_string(cif_rows) + " not "
             + std::to_string(arr.get_length()));
      if (cif_rows == 1) {
        items.emplace_back(tag, as_cif_value(arr.get_array_element(0)));
      } else {
        Loop& loop = items.back().loop;
        loop.tags.emplace_back(std::move(tag));
        for (size_t k = 0; k != cif_rows; ++k)
          loop.values[j + k*cif_cols] = as_cif_value(arr.get_array_element(k));
      }
    }
  }
}

// reads mmJSON file mutating the input buffer as a side effect
inline Document read_mmjson_insitu(char* buffer, size_t size,
                                   const std::string& name="mmJSON") {
  Document doc;
  sajson::document json = sajson::parse(sajson::dynamic_allocation(),
                                    sajson::mutable_string_view(size, buffer));
  if (!json.is_valid())
    fail(name + ":" + std::to_string(json.get_error_line()) + " error: " +
         json.get_error_message_as_string());
  fill_document_from_sajson(doc, json);
  doc.source = name;
  return doc;
}

inline Document read_mmjson(const std::string& path) {
  gemmi::fileptr_t f = gemmi::file_open(path.c_str(), "rb");
  size_t buf_size = gemmi::file_size(f.get(), path);
  std::vector<char> buffer(buf_size);
  if (std::fread(buffer.data(), buffer.size(), 1, f.get()) != 1)
    fail(path + ": fread failed");
  return read_mmjson_insitu(buffer.data(), buffer.size(), path);
}

template<typename T>
Document read_any_mmjson(const T& input) {
  /*
  if (input.is_stdin()) {
    size_t size = 16*1024;
    // getline + std::cin?
    return read_mmjson_insitu(stdin, size, "stdin");
  } */
  if (std::unique_ptr<char[]> mem = input.memory())
    return read_mmjson_insitu(mem.get(), input.mem_size(), input.path());
  return read_mmjson(input.path());
}

} // namespace cif
} // namespace gemmi
#endif
// vim:sw=2:ts=2:et
