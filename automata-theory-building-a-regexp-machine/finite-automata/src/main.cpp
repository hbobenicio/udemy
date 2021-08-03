#include <iostream>
#include <memory>
#include <optional>

#include "fa/nfa/state.h"
#include "fa/nfa/nfa.h"

using namespace std;
using namespace fa::nfa;

static void t1()
{
    cout << "T1:\n";
    {
        NFA regex{'a'};

        GraphDumpVisitor visitor{"T1: basic: single character: regex='a'"};
        regex.accept(visitor);
        visitor.dump_graph("/tmp/t1-basic-single-char.dot");
    }
    {
        // epsilon. S0 -e-> S1
        NFA regex;

        GraphDumpVisitor visitor{"T1: basic: epsilon: regex=ε"};
        regex.accept(visitor);
        visitor.dump_graph("/tmp/t1-basic-epsilon.dot");
    }
}

static void t2()
{
    cout << "T2:\n";

    NFA regex = concat(
        NFA{'a'},
        NFA{'b'},
        NFA{'c'}
    );

    GraphDumpVisitor visitor{"T2: operator: concat: regex='abc'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t2-op-concat.dot");
}

static void t3()
{
    cout << "T3:\n";

    NFA regex = disjoint(
        NFA{'a'},
        NFA{'b'},
        NFA{'c'}
    );

    GraphDumpVisitor visitor{"T3: operator: union '|': regex='a|b|c'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t3-op-union.dot");
}

static void t4()
{
    cout << "T4:\n";

    NFA regex = kleene(NFA{'a'});

    GraphDumpVisitor visitor{"T4: operator: kleene star '*': regex='a*'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t4-op-kleene.dot");
}

/**
 * xy*|z
 */
static void t5()
{
    cout << "T5:\n";

    NFA regex = disjoint(
        concat(
            NFA{'x'},
            kleene(NFA{'y'})
        ),
        NFA{'z'}
    );
    
    GraphDumpVisitor visitor{"T5: complex: precedence: regex='xy*|z'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t5-complex-precedence.dot");
}

static void t6()
{
    cout << "T6:\n";

    NFA regex = plus_naive(NFA{'a'});

    GraphDumpVisitor visitor{"T6: syntatic sugar: plus '+' naive approach: regex='a+'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t6-syntatic-sugar-plus-naive.dot");
}

static void t7()
{
    cout << "T7:\n";

    NFA regex = question_mark_naive(NFA{'a'});

    GraphDumpVisitor visitor{"T7: syntatic sugar: question mark '?' naive approach: regex='a?'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t7-syntatic-sugar-question-mark-naive.dot");
}

static void t8()
{
    cout << "T8:\n";

    NFA regex = digit_naive();
    
    GraphDumpVisitor visitor{"T8: syntatic sugar: digit character class naive approach: regex='[\\d]'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t8-syntatic-sugar-digit-naive.dot");
}

static void t9()
{
    cout << "T9:\n";

    NFA regex = char_range_naive('0', '3');
    
    GraphDumpVisitor visitor{"T9: syntatic sugar: character range/class naive approach: regex='[0-3]'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t9-syntatic-sugar-char-range-naive.dot");
}

// TODO syntatic sugar:
//    - /a+|[0-3]/ <=> /aa*|(0|1|2|3)/

int main()
{
    t1();
    t2();
    t3();
    t4();
    t5();
    t6();
    t7();
    t8();
    t9();

    return 0;
}