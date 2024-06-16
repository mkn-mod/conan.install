/**
Copyright (c) 2013, Philip Deegan.
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
#include "maiken/module/init.hpp"

namespace mkn {
namespace mod {
namespace conan_io {

class Exception : public kul::Exception {
public:
  Exception(const char *f, const uint16_t &l, const std::string &s)
      : kul::Exception(f, l, s) {}
};

class InstallModule;
class Validater {
  friend class InstallModule;

private:
  std::unordered_map<std::string, bool> is_valid;
  bool exists_or_is_valid(const std::string &s) {
    return is_valid.count(s) && is_valid[s];
  }
};

class ConanFile {
  friend class InstallModule;
};
class ConanFileMan {
  friend class InstallModule;

  std::unordered_map<std::string, ConanFile> files;

  ConanFile &get_or_create(const kul::File &file) { return files[file.real()]; }
};

class InstallModule : public maiken::Module {
private:
  ConanFileMan cfm;
  static void VALIDATE_NODE(YAML::Node const &node) {
    using namespace kul::yaml;
    Validator({NodeValidator("install")}).validate(node);
  }
  std::string python_exe = "python";

public:
  InstallModule() {
    std::string py(kul::env::GET("PYTHON"));
    if (!py.empty())
      python_exe = py;
  }
  void init(maiken::Application &a, YAML::Node const &node)
      KTHROW(std::exception) override {
    VALIDATE_NODE(node);

    kul::File conanFile("conanfile.txt", a.project().dir());
    if (!conanFile)
      return;
    ConanFile &cFile(cfm.get_or_create(conanFile));
    kul::io::Reader rdr(conanFile);

    std::string s;
    const char *line = 0;
    bool read = 0;
    while ((line = rdr.readLine())) {
      s = line;
      kul::String::TRIM(s);
      if (s == "[requires]") {
        read = 1;
        continue;
      }
      if (read && s[0] == '[')
        break;
      if (read) {
        if (s.find("/") == std::string::npos)
          KEXCEPTION("conanFile is invalid");
        auto bits = kul::String::SPLIT(s, "/");
        kul::Dir conan_dir = kul::user::home(".conan/data");
        kul::Dir req_dir(bits[0] + "/" + bits[1] + "/package", conan_dir);
        if (!req_dir) {
          std::string install = node["install"] ? node["install"].Scalar() : "";
          kul::os::PushDir pdir(a.project().dir());
          kul::Process p(python_exe);
          p << "-m conans.conan install" << install << ".";
          p.start();
        }
        auto req_dirs(req_dir.dirs());
        if (req_dirs.size() != 1)
          KEXCEPTION("conan data is inconsistent");
        kul::Dir req_home(req_dirs[0]);
        kul::Dir req_include("include", req_home);
        kul::Dir req_lib("lib", req_home);
        if (req_include) {
          a.addInclude(req_include.escr());
          for (auto *rep : a.revendencies())
            rep->addInclude(req_include.escr());
        }
        if (req_lib) {
          a.addLibpath(req_lib.escr());
          for (auto *rep : a.revendencies())
            rep->addLibpath(req_lib.escr());
        }
      }
    }
  }
  void link(maiken::Application &a, YAML::Node const &node)
      KTHROW(std::exception) override {
    kul::File conanFile("conanfile.txt", a.project().dir());
    if (!conanFile)
      return;
    ConanFile &cFile(cfm.get_or_create(conanFile));
  }
  void pack(maiken::Application &a, YAML::Node const &node)
      KTHROW(std::exception) override {
    kul::File conanFile("conanfile.txt", a.project().dir());
    if (!conanFile)
      return;
    ConanFile &cFile(cfm.get_or_create(conanFile));
  }
};
} // namespace conan_io
} // namespace mod
} // namespace mkn

extern "C" KUL_PUBLISH maiken::Module *maiken_module_construct() {
  return new mkn ::mod ::conan_io ::InstallModule;
}

extern "C" KUL_PUBLISH void maiken_module_destruct(maiken::Module *p) {
  delete p;
}
