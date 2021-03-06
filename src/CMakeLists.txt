
#project (gserver3 CXX C)

option(CPP_STARTER_USE_QT "Enable compilation of QT sample" OFF)
option(CPP_STARTER_USE_FLTK "Enable compilation of FLTK sample" OFF)
option(CPP_STARTER_USE_GTKMM "Enable compilation of GTKMM sample" OFF)
option(CPP_STARTER_USE_IMGUI "Enable compilation of ImGui sample" OFF)
option(CPP_STARTER_USE_NANA "Enable compilation of Nana GUI sample" OFF)
option(CPP_STARTER_USE_SDL "Enable compilation of SDL sample" OFF)

# sdl
if(CPP_STARTER_USE_SDL)
  message("Using SDL2")
  add_subdirectory(sdl)
endif()

# qt
if(CPP_STARTER_USE_QT)
  message("Using Qt")
  add_subdirectory(qt)
endif()

# fltk test
if(CPP_STARTER_USE_FLTK)
  message("Using FLTK")
  add_subdirectory(fltk)
endif()

# gtkmm test
if(CPP_STARTER_USE_GTKMM)
  message("Using gtkmm")
  add_subdirectory(gtkmm)
endif()

# imgui example
#if(CPP_STARTER_USE_IMGUI)
#  message("Using Dear ImGui")
#  add_subdirectory(imgui)
#endif()

# Nana
if(CPP_STARTER_USE_NANA)
  message("Using Nana")
  add_subdirectory(nana)
endif()

find_package(fmt)
find_package(spdlog)
find_package(docopt)
find_package(Taskflow)

add_executable(gserver3 main.cpp native_impl.cpp)

find_package(ImGui-SFML REQUIRED)

add_dependencies(gserver3 vmlib wasm-modules wsslib)

target_link_libraries(gserver3 PRIVATE project_options project_warnings Taskflow::Taskflow ImGui-SFML::ImGui-SFML docopt::docopt fmt::fmt spdlog::spdlog vmlib wsslib -lpthread -lm)

