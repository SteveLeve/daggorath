# Placeholder for the presentation module. It carries no sources yet; the interface target
# exists so dependents can be wired up before the implementation lands.
add_library(daggorath_presentation INTERFACE)
add_library(daggorath::presentation ALIAS daggorath_presentation)
