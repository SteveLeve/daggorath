# dcli: headless trace harness. Runs timestamped input scripts against the core
# and emits comparable state/event traces.
add_executable(dcli dcli.cpp)
target_link_libraries(dcli PRIVATE daggorath::core daggorath::presentation)

# Closed-loop Original Mode planner: records a power-on to WINNER dcli script.
add_executable(dplan dplan.cpp)
target_link_libraries(dplan PRIVATE daggorath::core)

# A desktop SDL3 application belongs here later, as a separate target that links
# daggorath::core plus daggorath::presentation and daggorath::platform.
