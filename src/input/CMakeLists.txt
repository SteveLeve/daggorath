# Placeholder for the input module. It carries no sources yet; the interface target
# exists so dependents can be wired up before the implementation lands.
add_library(daggorath_input INTERFACE)
add_library(daggorath::input ALIAS daggorath_input)
