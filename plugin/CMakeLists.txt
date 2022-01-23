find_package(Boost REQUIRED COMPONENTS system)

add_library(metacpp-plugin STATIC plugin.cpp)

target_link_libraries(metacpp-plugin PRIVATE metacpp::refl Boost::system)

add_library(metacpp::plugin ALIAS metacpp-plugin)
