# Tests link only against the Qt-free core library, so they run headless
# (no display server needed on CI).

add_executable(test_graph
    test_graph.cpp
    TestAssert.h)
target_link_libraries(test_graph PRIVATE gitscope_core gitscope_warnings)
add_test(NAME graph_builder COMMAND test_graph)

add_executable(test_repository
    test_repository.cpp
    TestAssert.h
    TestRepoFixture.h)
target_link_libraries(test_repository PRIVATE gitscope_core gitscope_warnings)
add_test(NAME repository COMMAND test_repository)
