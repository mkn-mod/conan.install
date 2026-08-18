/**
Copyright (c) 2026, Philip Deegan.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Philip Deegan nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "mkn/mod/init.hpp"  // IWYU pragma: keep

#include "mkn/kul/io.hpp"
#include "mkn/kul/os.hpp"
#include "mkn/kul/proc.hpp"
#include "mkn/kul/string.hpp"
#include "mkn/kul/yaml.hpp"

#include <string>

namespace mkn::mod::conan_io {

class Exception : public kul::Exception {
 public:
  Exception(char const* f, uint16_t const& l, std::string const& s) : kul::Exception(f, l, s) {}
};

class InstallModule;
class Validater {
  friend class InstallModule;

 private:
  std::unordered_map<std::string, bool> is_valid;
  bool exists_or_is_valid(std::string const& s) { return is_valid.count(s) && is_valid[s]; }
};

class ConanFile {
  friend class InstallModule;
};
class ConanFileMan {
  friend class InstallModule;

  std::unordered_map<std::string, ConanFile> files;

  ConanFile& get_or_create(kul::File const& file) { return files[file.real()]; }
};

class InstallModule : public mkn::mod::Module {
 public:
  void init(mkn::mod::Context& a, YAML::Node const& node) override {
    try {
      init_(a, node);
    } catch (kul::Exception const& e) {
      KLOG(ERR) << e.what();
      std::cout.flush();
      std::cerr.flush();
      std::exit(1);
    } catch (std::exception const& e) {
      KERR << e.what();
      std::cout.flush();
      std::cerr.flush();
      std::exit(1);
    } catch (...) {
      KERR << "conan.install: UNKNOWN EXCEPTION TYPE CAUGHT in init()";
      std::cout.flush();
      std::cerr.flush();
      std::exit(1);
    }
  }
  void link(mkn::mod::Context& a, YAML::Node const& node) override {
    try {
      kul::File conanFile("conanfile.txt", a.state().projectDir);
      if (!conanFile) return;
      ConanFile& cFile(cfm.get_or_create(conanFile));
    } catch (kul::Exception const& e) {
      KLOG(ERR) << e.what();
      std::cout.flush();
      std::cerr.flush();
      std::exit(1);
    } catch (std::exception const& e) {
      KERR << e.what();
      std::cout.flush();
      std::cerr.flush();
      std::exit(1);
    } catch (...) {
      KERR << "conan.install: UNKNOWN EXCEPTION TYPE CAUGHT in link()";
      std::cout.flush();
      std::cerr.flush();
      std::exit(1);
    }
  }
  void pack(mkn::mod::Context& a, YAML::Node const& node) override {
    try {
      kul::File conanFile("conanfile.txt", a.state().projectDir);
      if (!conanFile) return;
      ConanFile& cFile(cfm.get_or_create(conanFile));
    } catch (kul::Exception const& e) {
      KLOG(ERR) << e.what();
      std::cout.flush();
      std::cerr.flush();
      std::exit(1);
    } catch (std::exception const& e) {
      KERR << e.what();
      std::cout.flush();
      std::cerr.flush();
      std::exit(1);
    } catch (...) {
      KERR << "conan.install: UNKNOWN EXCEPTION TYPE CAUGHT in pack()";
      std::cout.flush();
      std::cerr.flush();
      std::exit(1);
    }
  }

 private:
  ConanFileMan cfm;
  std::string const python_exe = kul::env::GET("PYTHON", "python3");

  static void VALIDATE_NODE(YAML::Node const& node) {
    using namespace kul::yaml;
    Validator({NodeValidator("install")}).validate(node);
  }

  auto parse_cmake_var(std::string const& find, std::string const& s) {
    std::vector<std::string> data;
    std::size_t const offset = s.find(find) + find.size() + 1;
    auto const line = s.substr(offset, s.size() - offset);
    for (auto const& b : kul::String::ESC_SPLIT(line, ' '))
      data.emplace_back(b.substr(1, b.size() - 2));
    for (auto& l : data)
      if (l[l.size() - 1] == '"') l = l.substr(0, l.size() - 1);
    return data;
  }

  auto parse_for_lib_and_incs(kul::File const& toolChainFile) {
    kul::io::Reader r(toolChainFile);
    std::vector<std::string> inc, lib;
    char const* c = nullptr;
    while ((c = r.readLine())) {
      std::string str = c;
      if (inc.size() == 0 && str.find("CMAKE_INCLUDE_PATH") != std::string::npos)
        inc = parse_cmake_var("CMAKE_INCLUDE_PATH", str);
      if (lib.size() == 0 && str.find("CMAKE_LIBRARY_PATH") != std::string::npos)
        lib = parse_cmake_var("CMAKE_LIBRARY_PATH", str);
      if (inc.size() && lib.size()) break;
    }
    return std::make_tuple(inc, lib);
  }

  void init_(mkn::mod::Context& a, YAML::Node const& node) {
    VALIDATE_NODE(node);

    auto const& projectDir = a.state().projectDir;

    kul::File conanFile("conanfile.txt", projectDir);
    if (!conanFile) return;

    kul::Dir buildDir{"build", kul::Dir(projectDir)};
    kul::Dir generatorDir{"generators", buildDir};
    kul::File conanToolChainFile("conan_toolchain.cmake", generatorDir);

    ConanFile& cFile(cfm.get_or_create(conanFile));
    kul::io::Reader rdr(conanFile);

    std::string s;
    char const* line = 0;
    bool read = 0;
    while ((line = rdr.readLine())) {
      s = line;
      kul::String::TRIM(s);
      if (s == "[requires]") {
        read = 1;
        continue;
      }
      if (read && s[0] == '[') break;
      if (read) {
        if (s.find("/") == std::string::npos) KEXCEPTION("conanFile is invalid");
        auto bits = kul::String::SPLIT(s, "/");
        kul::Dir conan_dir = kul::user::home(".conan/data");

        if (!conanToolChainFile) {
          std::string install = node["install"] ? node["install"].Scalar() : "";
          kul::os::PushDir pdir(projectDir);
          kul::Process p(python_exe);
          p << "-m conans.conan install" << install << ". --build=missing";
          p.start();
        }

        auto const& [inc, lib] = parse_for_lib_and_incs(conanToolChainFile);

        for (auto const& d : inc) {
          kul::Dir req_include(d);
          if (req_include) {
            a.compilerState().add(IncludeInput(req_include.escr()));
            for (auto* dep : a.state().dependents)
              dep->compilerState().add(IncludeInput(req_include.escr()));
          }
        }

        for (auto const& d : lib) {
          kul::Dir req_lib(d);
          if (req_lib) {
            a.compilerState().add(LibPathInput(req_lib.escr()));
            for (auto* dep : a.state().dependents)
              dep->compilerState().add(LibPathInput(req_lib.escr()));
          }
        }
      }
    }
  }
};

}  // namespace mkn::mod::conan_io

extern "C" MKN_KUL_PUBLISH mkn::mod::Module* maiken_module_construct() {
  return new mkn ::mod ::conan_io ::InstallModule;
}

extern "C" MKN_KUL_PUBLISH void maiken_module_destruct(mkn::mod::Module* p) { delete p; }
