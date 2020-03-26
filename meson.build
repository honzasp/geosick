project('geosick', 'cpp')
cpp = meson.get_compiler('cpp')


mysql_dep = dependency('mysqlclient')

mysqlpp_includes = include_directories('/usr/include/mysql++')
mysqlpp_lib = cpp.find_library('mysqlpp')
mysqlpp_dep = declare_dependency(
  include_directories: mysqlpp_includes,
  dependencies: [mysqlpp_lib]
)

stdcppfs_dep = cpp.find_library('stdc++fs')
boost_dep = dependency('boost')


sources = files(
  'src/geosick/geo_distance.cpp',
  'src/geosick/geo_search.cpp',
  'src/geosick/main_zostanzdravy.cpp',
  'src/geosick/match.cpp',
  'src/geosick/mysql_db.cpp',
  'src/geosick/notify_process.cpp',
  'src/geosick/read_process.cpp',
  'src/geosick/sampler.cpp',
  'src/geosick/search_process.cpp',
)
includes = include_directories(
  'src',
  'extern/include',
)

executable('zostanzdravy', sources,
  include_directories: includes,
  dependencies: [mysql_dep, mysqlpp_dep, stdcppfs_dep, boost_dep],
  override_options: ['cpp_std=c++17'],
  cpp_args: ['-Wextra'],
)
