cmake_minimum_required(VERSION 3.8)

project(DynamicVariant)

find_package(Boost REQUIRED unit_test_framework iostreams)
find_package(gulachek_gtree REQUIRED)

set(lib "gulachek_dynamic_variant")
add_library(${lib} INTERFACE)

target_compile_features(${lib}
	INTERFACE cxx_std_17
	)

target_include_directories(${lib} INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
	$<INSTALL_INTERFACE:include>
	)

target_link_libraries(${lib}
	INTERFACE gulachek_gtree
	)

add_executable(dv_test
	test/dynamic_variant_test.cpp
	)

target_link_libraries(dv_test
	PRIVATE ${lib}
	PRIVATE Boost::unit_test_framework
	)

set(export_name gulachek_dynamic_variant-config)
install(TARGETS ${lib} EXPORT ${export_name})

install(DIRECTORY include/gulachek/
	DESTINATION include/gulachek
	)

install(EXPORT ${export_name}
	DESTINATION lib/cmake/${export_name}
	)
