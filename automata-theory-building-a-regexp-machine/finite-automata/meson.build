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
    'src/main.cpp',
]

finite_automata = executable('finite-automata', sources,
    include_directories: [
        include_directories('src')
    ],
)
