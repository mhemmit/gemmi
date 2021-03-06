// Copyright 2020 Global Phasing Ltd.
//
// A function that generates biological assemblies by applying operations
// from struct Assembly to a Model.

#ifndef GEMMI_ASSEMBLY_HPP_
#define GEMMI_ASSEMBLY_HPP_

#include <ostream>      // std::ostream
#include "model.hpp"
#include "util.hpp"

namespace gemmi {

enum class HowToNameCopiedChains { Short, AddNumber, Dup };

struct ChainNameGenerator {
  using How = HowToNameCopiedChains;
  How how;
  std::vector<std::string> used_names;

  ChainNameGenerator(How how_) : how(how_) {}
  ChainNameGenerator(const Model& model, How how_) : how(how_) {
    if (how != How::Dup)
      for (const Chain& chain : model.chains)
        used_names.push_back(chain.name);
  }
  bool has(const std::string& name) const {
    return in_vector(name, used_names);
  }
  const std::string& added(const std::string& name) {
    used_names.push_back(name);
    return name;
  }

  std::string make_short_name(const std::string& preferred) {
    static const char symbols[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "abcdefghijklmnopqrstuvwxyz0123456789";
    if (!has(preferred))
      return added(preferred);
    std::string name(1, 'A');
    for (char symbol : symbols) {
      name[0] = symbol;
      if (!has(name))
        return added(name);
    }
    name += 'A';
    for (char symbol1 : symbols) {
      name[0] = symbol1;
      for (char symbol2 : symbols) {
        name[1] = symbol2;
        if (!has(name))
          return added(name);
      }
    }
    fail("run out of 1- and 2-letter chain names");
  }

  std::string make_name_with_numeric_postfix(const std::string& base, int n) {
    std::string name = base;
    name += std::to_string(n);
    while (has(name)) {
      name.resize(base.size());
      name += std::to_string(++n);
    }
    return added(name);
  }

  std::string make_new_name(const std::string& old, int n) {
    switch (how) {
      case How::Short: return make_short_name(old);
      case How::AddNumber: return make_name_with_numeric_postfix(old, n);
      case How::Dup: return old;
    }
    unreachable();
  }
};

inline Model make_assembly(const Assembly& assembly, const Model& model,
                           HowToNameCopiedChains how, std::ostream* out) {
  Model new_model(model.name);
  ChainNameGenerator namegen(how);
  std::map<std::string, std::string> subs = model.subchain_to_chain();
  for (const Assembly::Gen& gen : assembly.generators)
    for (const Assembly::Oper& oper : gen.opers) {
      if (out) {
        *out << "Applying " << oper.name << " to";
        if (!gen.chains.empty())
          *out << " chains: " << join_str(gen.chains, ',');
        else if (!gen.subchains.empty())
          *out << " subchains: " << join_str(gen.subchains, ',');
        *out << std::endl;
        for (const std::string& chain_name : gen.chains)
          if (!model.find_chain(chain_name))
            *out << "Warning: no chain " << chain_name << std::endl;
        for (const std::string& subchain_name : gen.subchains)
          if (subs.find(subchain_name) == subs.end())
            *out << "Warning: no subchain " << subchain_name << std::endl;
      }
      // PDB files specify bioassemblies in terms of chains,
      // mmCIF files in terms of subchains. We handle the two cases separately.
      if (!gen.chains.empty()) {
        // chains are not merged here, multiple chains may have the same name
        std::map<std::string, std::string> new_names;
        for (size_t i = 0; i != model.chains.size(); ++i) {
          if (in_vector(model.chains[i].name, gen.chains)) {
            new_model.chains.push_back(model.chains[i]);
            Chain& new_chain = new_model.chains.back();
            auto name_iter = new_names.find(model.chains[i].name);
            if (name_iter == new_names.end()) {
              new_chain.name = namegen.make_new_name(new_chain.name, 1);
              new_names.emplace(model.chains[i].name, new_chain.name);
            } else {
              new_chain.name = name_iter->second;
            }
            for (Residue& res : new_chain.residues) {
              for (Atom& a : res.atoms)
                a.pos = Position(oper.transform.apply(a.pos));
              if (!res.subchain.empty())
                res.subchain = new_chain.name + ":" + res.subchain;
            }
          }
        }
      } else if (!gen.subchains.empty()) {
        std::map<std::string, std::string> new_names;
        for (const std::string& subchain_name : gen.subchains) {
          auto sub_iter = subs.find(subchain_name);
          if (sub_iter == subs.end())
            continue;
          auto name_iter = new_names.find(sub_iter->second);
          Chain* new_chain;
          if (name_iter == new_names.end()) {
            std::string new_name = namegen.make_new_name(sub_iter->second, 1);
            new_names.emplace(sub_iter->second, new_name);
            new_model.chains.emplace_back(new_name);
            new_chain = &new_model.chains.back();
          } else {
            new_chain = new_model.find_chain(name_iter->second);
          }
          for (const Residue& res : model.get_subchain(subchain_name)) {
            new_chain->residues.push_back(res);
            Residue& new_res = new_chain->residues.back();
            new_res.subchain = new_chain->name + ":" + res.subchain;
            for (Atom& a : new_res.atoms)
              a.pos = Position(oper.transform.apply(a.pos));
          }
        }
      }
    }
  return new_model;
}

inline void change_to_assembly(Structure& st, const std::string& assembly_name,
                               HowToNameCopiedChains how, std::ostream* out) {
  Assembly* assembly = st.find_assembly(assembly_name);
  if (!assembly) {
    if (st.assemblies.empty())
      fail("no bioassemblies are listed for this structure");
    fail("wrong assembly name, use one of: " +
        join_str(st.assemblies, ' ', [](const Assembly& a) { return a.name; }));
  }
  for (Model& model : st.models)
    model = make_assembly(*assembly, model, how, out);
  st.connections.clear();
}

} // namespace gemmi
#endif
