pymod = import('python')
py = pymod.find_installation('python3')
py.extension_module('_geosick',
  sources : [
    'py.cpp'
  ],
  dependencies : [
    dependency('python3'),
    dependency('libpq')
  ],
  install : true,
  subdir : ''
)

