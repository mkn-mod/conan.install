
#include "mkn/kul/defs.hpp"
#include "mkn/kul/os.hpp"
#include "mkn/kul/signal.hpp"
#include "mkn/kul/yaml.hpp"
#include "mkn/mod/loader.hpp"

#if MKN_KUL_IS_WIN
std::string const yArgs = R"(install: -s compiler="msvc")";
#else
std::string const yArgs = "";
#endif

namespace {

class FakeContext : public mkn::mod::Context, public mkn::mod::CompilerState {
 public:
  mkn::mod::ContextState state() const override {
    mkn::mod::ContextState s;
    s.projectDir = dir;
    s.includes = incs;
    s.dependents = rdeps;
    s.buildMode = m;
    return s;
  }
  mkn::mod::CompilerState& compilerState() override { return *this; }
  void per_compiler_command(CompileHook) override {}
  std::string compileCommandFor(std::string const&) const override { return ""; }

  void add(mkn::mod::AbstractCompilerInput const& input) override {
    if (auto const* i = dynamic_cast<mkn::mod::IncludeInput const*>(&input))
      incs.emplace_back(i->path, i->is_public);
    else if (auto const* i = dynamic_cast<mkn::mod::LibPathInput const*>(&input))
      libpaths.emplace_back(i->path);
    else if (auto const* i = dynamic_cast<mkn::mod::BuildModeInput const*>(&input))
      m = i->mode;
  }

 private:
  std::string dir{"."};
  std::vector<std::pair<std::string, bool>> incs;
  std::vector<std::string> libpaths;
  std::vector<mkn::mod::Context*> rdeps;
  mkn::mod::Mode m = mkn::mod::Mode::SHAR;
};

mkn::kul::File find_module() {
  mkn::kul::Dir dir("bin/build");
  for (auto const& f : dir.files(0)) {
    auto const& name = f.name();
    auto const dot = name.rfind(".");
    if (dot == std::string::npos) continue;
#if MKN_KUL_IS_WIN
    if (name.substr(dot + 1) == "dll") return mkn::kul::File(f.real());
#else
    if (name.substr(dot + 1) == "so") return mkn::kul::File(f.real());
#endif
  }
  KEXCEPTION("No loadable module found in ", dir.escr());
}

}  // namespace

int main() {
  KOUT(NON) << __FILE__;
  mkn::kul::Signal sig;
  try {
    YAML::Node node = mkn::kul::yaml::String(yArgs).root();
    mkn::mod::Loader loader(find_module());
    FakeContext ctx;
    loader.module()->init(ctx, node);
    loader.module()->compile(ctx, node);
    loader.module()->link(ctx, node);
    loader.module()->pack(ctx, node);
    loader.unload();
  } catch (mkn::kul::Exception const& e) {
    KLOG(ERR) << e.what();
    return 2;
  } catch (std::exception const& e) {
    KERR << "EXCEPTION: " << e.what();
    return 3;
  } catch (...) {
    KERR << "UNKNOWN EXCEPTION TYPE CAUGHT";
    return 5;
  }
  return 0;
}
