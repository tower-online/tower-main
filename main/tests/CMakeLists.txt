add_executable(tower-integration-tests
    db/psql_connection.test.cpp
)
target_compile_features(tower-integration-tests PRIVATE cxx_std_20)
target_link_libraries(tower-integration-tests PRIVATE tower::libtower Catch2::Catch2WithMain)