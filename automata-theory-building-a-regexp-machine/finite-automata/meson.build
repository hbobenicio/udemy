# For using the clang sanitizer:
# $ cd builddir
# $ meson configure -Db_sanitizer=address -Db_lundef=false # you can check if it is ok with `meson configure`
# $ meson compile
# $ ./finite-automata
project('finite-automata', 'cpp',
    version : '0.1.0',
    default_options : [
        'warning_level=3',
        'cpp_std=c++17',
        'default_library=both'
    ]
)

sources = [
    'src/fa/nfa/state.cpp',
    'src/fa/nfa/nfa.cpp',
    'src/fa/nfa/graph.cpp',
    'src/fa/dfa/dfa.cpp',
    'src/main.cpp',
]

finite_automata = executable('finite-automata', sources,
    include_directories: [
        include_directories('src')
    ],
)
