add_executable(tower-integration-tests
    collision.test.cpp
    transform.test.cpp
)

target_compile_features(tower-integration-tests PRIVATE cxx_std_23)
target_link_libraries(tower-integration-tests PRIVATE Catch2::Catch2WithMain tower::libtower)