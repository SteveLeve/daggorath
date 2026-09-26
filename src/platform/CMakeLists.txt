# Placeholder for the platform module. It carries no sources yet; the interface target
# exists so dependents can be wired up before the implementation lands.
add_library(daggorath_platform INTERFACE)
add_library(daggorath::platform ALIAS daggorath_platform)
