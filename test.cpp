

#include "mkn/kul/signal.hpp"

#include <maiken.hpp>

int main() {
  kul::Signal sig;
  try {
    char* argv[]{"program", "-R"};
    auto app = (maiken::Application::CREATE(2, argv2))[0];
    auto loader(maiken::ModuleLoader::LOAD(*app));
    loader->module()->link(*app, node);
    loader->unload();
  } catch (const kul::Exception& e) {
    KERR << e.what();
    return 2;
  } catch (const std::exception& e) {
    KERR << e.what();
    return 3;
  }
  return 0;
}
