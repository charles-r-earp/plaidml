// Copyright 2018, Intel Corporation

#include "tile/codegen/driver.h"

#include <boost/format.hpp>

#include "base/config/config.h"
#include "base/util/any_factory_map.h"
#include "base/util/throw.h"
#include "tile/codegen/alias.h"
#include "tile/codegen/compile_pass.h"
#include "tile/codegen/emitc.h"

namespace vertexai {
namespace tile {
namespace codegen {

using namespace stripe;  // NOLINT

namespace {

void DumpProgram(const Block& program,            //
                 const OptimizeOptions& options,  //
                 const std::string& name,         //
                 size_t counter) {
  if (options.dump_passes || options.dump_code) {
    boost::filesystem::create_directory(options.dbg_dir);
    if (options.dump_passes) {
      auto filename = str(boost::format("%02zu_%s.txt") % counter % name);
      auto path = (options.dbg_dir / filename).string();
      std::ofstream fout(path);
      fout << program << std::endl;
    }
    if (options.dump_code) {
      auto filename = str(boost::format("%02zu_%s.c") % counter % name);
      auto path = (options.dbg_dir / filename).string();
      std::ofstream fout(path);
      fout << EmitC(program);
    }
  }
}

void ValidateBlock(Block* root) {
  RunOnBlocks(  //
      root, {},
      [&](auto map, auto block) {
        for (const auto& ref : block->refs) {
          if (ref.dir == RefDir::None && !ref.from.empty()) {
            throw_with_trace(std::runtime_error(
                str(boost::format("ref.dir == RefDir::None && !ref.from.empty(). ref: %1% in block: %2%") % ref.into() %
                    block->name)));
          }
          if (ref.from.empty() && ref.dir != RefDir::None) {
            throw_with_trace(std::runtime_error(
                str(boost::format("ref.from.empty() && ref.dir != RefDir::None. ref: %1% in block: %2%") % ref.into() %
                    block->name)));
          }
        }
      },
      true);
}

class ConfigsRegistry {
 public:
  static ConfigsRegistry* Instance() {
    static ConfigsRegistry registry;
    return &registry;
  }

  void Register(const std::string& name, const std::string& cfg_bytes) {  //
    registry_[name] = cfg_bytes;
  }

  proto::Config Resolve(const std::string& name) {
    auto it = registry_.find(name);
    if (it == registry_.end()) {
      throw_with_trace(std::runtime_error(str(boost::format("Could not find config: %s") % name)));
    }
    return ParseConfig<proto::Config>(it->second);
  }

 private:
  std::unordered_map<std::string, std::string> registry_;
};

}  // namespace

void Optimize(Block* block, const Passes& passes, const OptimizeOptions& options) {
  size_t counter = 0;
  DumpProgram(*block, options, "initial", counter++);
  for (const auto& pass : passes) {
    IVLOG(1, "Optimization Pass " << pass.name());
    std::unique_ptr<CompilePass> compile_pass =
        AnyFactoryMap<CompilePass>::Instance()->MakeInstanceIfSupported(context::Context{}, pass.pass());
    if (!compile_pass) {
      throw_with_trace(std::runtime_error(
          str(boost::format("Unsupported pass: %1% -> %2%") % pass.name() % pass.pass().type_url())));
    }
    compile_pass->Apply(block);
    DumpProgram(*block, options, pass.name(), counter++);
    ValidateBlock(block);
  }
}

void Configs::Register(const std::string& name, const std::string& pb_bytes) {
  ConfigsRegistry::Instance()->Register(name, pb_bytes);
}

proto::Config Configs::Resolve(const std::string& name) {  //
  return ConfigsRegistry::Instance()->Resolve(name);
}

}  // namespace codegen
}  // namespace tile
}  // namespace vertexai
