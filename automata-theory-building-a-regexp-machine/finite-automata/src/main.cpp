#include <iostream>
#include <memory>
#include <optional>
#include <cassert>

#include "fa/nfa/state.h"
#include "fa/nfa/nfa.h"

using namespace std;
using namespace fa::nfa;

static void t1()
{
    cout << "T1: ";
    {
        NFA regex{'a'};
        assert(regex.matches("a"));
        assert(!regex.matches(""));
        assert(!regex.matches("b"));

        GraphDumpVisitor visitor{"T1: basic: single character: regex='a'"};
        regex.accept(visitor);
        visitor.dump_graph("/tmp/t1-basic-single-char.dot");
    }
    {
        // epsilon. S0 -e-> S1
        NFA regex;
        assert(regex.matches(""));
        assert(!regex.matches("a"));

        GraphDumpVisitor visitor{"T1: basic: epsilon: regex=ε"};
        regex.accept(visitor);
        visitor.dump_graph("/tmp/t1-basic-epsilon.dot");
    }

    cout << "OK!\n";
}

static void t2()
{
    cout << "T2: ";

    NFA regex = concat(
        NFA{'a'},
        NFA{'b'},
        NFA{'c'}
    );
    assert(regex.matches("abc"));

    cout << "OK!\n";

    GraphDumpVisitor visitor{"T2: operator: concat: regex='abc'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t2-op-concat.dot");
}

static void t3()
{
    cout << "T3: ";

    NFA regex = disjoint(
        NFA{'a'},
        NFA{'b'},
        NFA{'c'}
    );
    assert(regex.matches("a"));
    assert(regex.matches("b"));
    assert(regex.matches("c"));

    cout << "OK!\n";

    GraphDumpVisitor visitor{"T3: operator: union '|': regex='a|b|c'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t3-op-union.dot");
}

static void t4()
{
    cout << "T4: ";

    NFA regex = kleene_naive(NFA{'a'});
    assert(regex.matches(""));
    assert(regex.matches("a"));
    assert(regex.matches("aa"));
    assert(regex.matches("aaa"));
    assert(!regex.matches("b"));

    cout << "OK!\n";

    GraphDumpVisitor visitor{"T4: operator: kleene_naive star '*': regex='a*'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t4-op-kleene-naive.dot");
}

/**
 * xy*|z
 */
static void t5()
{
    cout << "T5: ";

    NFA regex = disjoint(
        concat(
            NFA{'x'},
            kleene_naive(NFA{'y'})
        ),
        NFA{'z'}
    );
    assert(regex.matches("z"));
    assert(regex.matches("x"));
    assert(regex.matches("xy"));
    assert(regex.matches("xyy"));
    assert(regex.matches("xyyy"));
    assert(!regex.matches("y"));
    assert(!regex.matches("xz"));

    cout << "OK!\n";
    
    GraphDumpVisitor visitor{"T5: complex: precedence: regex='xy*|z'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t5-complex-precedence.dot");
}

static void t6()
{
    cout << "T6: ";

    NFA regex = plus_naive(NFA{'a'});
    assert(!regex.matches(""));
    assert(regex.matches("a"));
    assert(regex.matches("aa"));
    assert(regex.matches("aaa"));
    assert(!regex.matches("b"));

    cout << "OK!\n";

    GraphDumpVisitor visitor{"T6: syntatic sugar: plus '+' naive approach: regex='a+'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t6-syntatic-sugar-plus-naive.dot");
}

static void t7()
{
    cout << "T7: ";

    NFA regex = question_mark_naive(NFA{'a'});
    assert(regex.matches(""));
    assert(regex.matches("a"));
    assert(!regex.matches("aa"));
    assert(!regex.matches("b"));

    cout << "OK!\n";

    GraphDumpVisitor visitor{"T7: syntatic sugar: question mark '?' naive approach: regex='a?'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t7-syntatic-sugar-question-mark-naive.dot");
}

static void t8()
{
    cout << "T8: ";

    NFA regex = digit_naive();
    for (size_t i = 0; i < 10; i++) {
        string input = to_string(i);
        assert(regex.matches(input));
    }
    assert(!regex.matches(""));
    assert(!regex.matches("a"));

    cout << "OK!\n";
    
    GraphDumpVisitor visitor{"T8: syntatic sugar: digit character class naive approach: regex='[\\d]'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t8-syntatic-sugar-digit-naive.dot");
}

static void t9()
{
    cout << "T9: ";

    NFA regex = char_range_naive('1', '3');
    assert(!regex.matches("0"));
    assert(regex.matches("1"));
    assert(regex.matches("2"));
    assert(regex.matches("3"));
    assert(!regex.matches("4"));

    cout << "OK!\n";
    
    GraphDumpVisitor visitor{"T9: syntatic sugar: character range/class naive approach: regex='[0-3]'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t9-syntatic-sugar-char-range-naive.dot");
}

static void t10()
{
    cout << "T10: ";

    NFA regex = zeroOrMore(NFA{'a'});
    assert(regex.matches(""));
    assert(regex.matches("a"));
    assert(regex.matches("aa"));
    assert(regex.matches("aaa"));
    assert(!regex.matches("b"));

    cout << "OK!\n";

    GraphDumpVisitor visitor{"T10: optimizations: kleene (zeroOrMore): regex='a*'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t10-optimization-kleene.dot");
}

static void t11()
{
    cout << "T11: ";

    NFA regex = oneOrMore(NFA{'a'});
    assert(!regex.matches(""));
    assert(regex.matches("a"));
    assert(regex.matches("aa"));
    assert(regex.matches("aaa"));
    assert(!regex.matches("b"));

    cout << "OK!\n";

    GraphDumpVisitor visitor{"T11: optimizations: plus '+' op (oneOrMore): regex='a+'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t11-optimization-plus.dot");
}

static void t12()
{
    cout << "T12: ";

    NFA regex = opt(NFA{'a'});
    assert(regex.matches(""));
    assert(regex.matches("a"));
    assert(!regex.matches("aa"));
    assert(!regex.matches("b"));

    cout << "OK!\n";

    GraphDumpVisitor visitor{"T12: optimizations: question mark '?' op (optional): regex='a?'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t12-optimization-question-mark.dot");
}

static void t13()
{
    cout << "T13: ";

    NFA regex = range('0', '9');
    for (size_t i = 0; i < 10; i++) {
        string input = to_string(i);
        assert(regex.matches(input));
    }
    assert(!regex.matches(""));
    assert(!regex.matches("a"));

    cout << "OK!\n";

    GraphDumpVisitor visitor{"T13: optimizations: range char class op: regex='[x-y]'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t13-optimization-range.dot");
}

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
    t10();
    t11();
    t12();
    t13();

    return 0;
}