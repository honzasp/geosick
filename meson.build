project('geosick', 'cpp')
cpp = meson.get_compiler('cpp')

mysql_dep = dependency('mysqlclient')

mysqlpp_includes = include_directories('/usr/include/mysql++')
mysqlpp_lib = cpp.find_library('mysqlpp')
mysqlpp_dep = declare_dependency(
  include_directories: mysqlpp_includes,
  dependencies: mysqlpp_lib,
)

sources = files(
  'src/geosick/geo_search.cpp',
  'src/geosick/main.cpp',
  'src/geosick/mysql_reader.cpp',
  'src/geosick/preprocess.cpp',
  'src/geosick/sampler.cpp',
)
includes = include_directories('src')

executable('geosick', sources,
  include_directories: includes,
  dependencies: [mysql_dep, mysqlpp_dep],
  override_options: ['cpp_std=c++17'],
)
