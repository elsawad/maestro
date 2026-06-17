add_executable(cli_server
  main.cc
)

target_link_libraries(cli_server
  PRIVATE
    maestro
)
