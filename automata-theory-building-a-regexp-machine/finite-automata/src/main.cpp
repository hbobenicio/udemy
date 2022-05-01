#include <iostream>
#include <memory>
#include <optional>
#include <cassert>

#include "fa/nfa/state.h"
#include "fa/nfa/nfa.h"

using namespace std;
using namespace fa::nfa;

static void test_fundamental_machines()
{
    cout << __func__ << ": ";
    {
        NFA regex{'a'};
        assert(regex.matches("a"));
        assert(!regex.matches(""));
        assert(!regex.matches("b"));

        GraphDumpVisitor visitor{"basic: single character: regex='a'"};
        regex.accept(visitor);
        visitor.dump_graph("/tmp/t1-basic-single-char.dot");
    }
    {
        // epsilon. S0 -e-> S1
        NFA regex;
        assert(regex.matches(""));
        assert(!regex.matches("a"));

        GraphDumpVisitor visitor{"basic: epsilon: regex=ε"};
        regex.accept(visitor);
        visitor.dump_graph("/tmp/t1-basic-epsilon.dot");
    }

    cout << "OK.\n";
}

static void test_operator_concat()
{
    cout << __func__ << ": ";

    NFA regex = concat(
        NFA{'a'},
        NFA{'b'},
        NFA{'c'}
    );
    assert(regex.matches("abc"));

    cout << "OK.\n";

    GraphDumpVisitor visitor{"operator: concat: regex='abc'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t2-op-concat.dot");
}

static void test_operator_union()
{
    cout << __func__ << ": ";

    NFA regex = disjoint(
        NFA{'a'},
        NFA{'b'},
        NFA{'c'}
    );
    assert(regex.matches("a"));
    assert(regex.matches("b"));
    assert(regex.matches("c"));

    cout << "OK.\n";

    GraphDumpVisitor visitor{"operator: union '|': regex='a|b|c'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t3-op-union.dot");
}

static void test_operator_kleene_naive()
{
    cout << __func__ << ": ";

    NFA regex = kleene_naive(NFA{'a'});
    assert(regex.matches(""));
    assert(regex.matches("a"));
    assert(regex.matches("aa"));
    assert(regex.matches("aaa"));
    assert(!regex.matches("b"));

    cout << "OK.\n";

    GraphDumpVisitor visitor{"operator: kleene_naive star '*': regex='a*'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t4-op-kleene-naive.dot");
}

/**
 * xy*|z
 */
static void test_complex_precedence()
{
    cout << __func__ << ": ";

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

    cout << "OK.\n";
    
    GraphDumpVisitor visitor{"complex: precedence: regex='xy*|z'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t5-complex-precedence.dot");
}

static void test_sugar_plus_naive()
{
    cout << __func__ << ": ";

    NFA regex = plus_naive(NFA{'a'});
    assert(!regex.matches(""));
    assert(regex.matches("a"));
    assert(regex.matches("aa"));
    assert(regex.matches("aaa"));
    assert(!regex.matches("b"));

    cout << "OK.\n";

    GraphDumpVisitor visitor{"syntatic sugar: plus '+' naive approach: regex='a+'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t6-syntatic-sugar-plus-naive.dot");
}

static void test_sugar_question_mark_naive()
{
    cout << __func__ << ": ";

    NFA regex = question_mark_naive(NFA{'a'});
    assert(regex.matches(""));
    assert(regex.matches("a"));
    assert(!regex.matches("aa"));
    assert(!regex.matches("b"));

    cout << "OK.\n";

    GraphDumpVisitor visitor{"syntatic sugar: question mark '?' naive approach: regex='a?'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t7-syntatic-sugar-question-mark-naive.dot");
}

static void test_sugar_digit_char_class_naive()
{
    cout << __func__ << ": ";

    NFA regex = digit_naive();
    for (size_t i = 0; i < 10; i++) {
        string input = to_string(i);
        assert(regex.matches(input));
    }
    assert(!regex.matches(""));
    assert(!regex.matches("a"));

    cout << "OK.\n";
    
    GraphDumpVisitor visitor{"syntatic sugar: digit character class naive approach: regex='[\\d]'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t8-syntatic-sugar-digit-naive.dot");
}

static void test_sugar_char_range_naive()
{
    cout << __func__ << ": ";

    NFA regex = char_range_naive('1', '3');
    assert(!regex.matches("0"));
    assert(regex.matches("1"));
    assert(regex.matches("2"));
    assert(regex.matches("3"));
    assert(!regex.matches("4"));

    cout << "OK.\n";
    
    GraphDumpVisitor visitor{"syntatic sugar: character range/class naive approach: regex='[0-3]'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t9-syntatic-sugar-char-range-naive.dot");
}

static void test_optimizations_operator_kleene()
{
    cout << __func__ << ": ";

    NFA regex = zeroOrMore(NFA{'a'});
    assert(regex.matches(""));
    assert(regex.matches("a"));
    assert(regex.matches("aa"));
    assert(regex.matches("aaa"));
    assert(!regex.matches("b"));

    cout << "OK.\n";

    GraphDumpVisitor visitor{"optimizations: kleene (zeroOrMore): regex='a*'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t10-optimization-kleene.dot");
}

static void test_optimizations_operator_plus()
{
    cout << __func__ << ": ";

    NFA regex = oneOrMore(NFA{'a'});
    assert(!regex.matches(""));
    assert(regex.matches("a"));
    assert(regex.matches("aa"));
    assert(regex.matches("aaa"));
    assert(!regex.matches("b"));

    cout << "OK.\n";

    GraphDumpVisitor visitor{"optimizations: plus '+' op (oneOrMore): regex='a+'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t11-optimization-plus.dot");
}

static void test_optimizations_operator_question_mark()
{
    cout << __func__ << ": ";

    NFA regex = opt(NFA{'a'});
    assert(regex.matches(""));
    assert(regex.matches("a"));
    assert(!regex.matches("aa"));
    assert(!regex.matches("b"));

    cout << "OK.\n";

    GraphDumpVisitor visitor{"optimizations: question mark '?' op (optional): regex='a?'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t12-optimization-question-mark.dot");
}

static void test_optimizations_operator_char_range()
{
    cout << __func__ << ": ";

    NFA regex = range('0', '9');
    for (size_t i = 0; i < 10; i++) {
        string input = to_string(i);
        assert(regex.matches(input));
    }
    assert(!regex.matches(""));
    assert(!regex.matches("a"));

    cout << "OK.\n";

    GraphDumpVisitor visitor{"optimizations: range char class op: regex='[x-y]'"};
    regex.accept(visitor);
    visitor.dump_graph("/tmp/t13-optimization-range.dot");
}

static void test_epsilon_closure()
{
    cout << __func__ << ": ";
    {
        auto s1 = make_shared<State>();
        auto s2 = make_shared<State>();
        auto s = make_shared<State>();

        s->add_transition(EPSILON, s1);
        s->add_transition(EPSILON, s2);

        const auto epsilon_closure = s->get_epsilon_closure();

        assert(epsilon_closure.size() == 3);
        assert(epsilon_closure[0] == s.get());
        assert(epsilon_closure[1] == s1.get());
        assert(epsilon_closure[2] == s2.get());
    }
    {
        NFA regex = NFA{'a'} | NFA{'b'};

        const auto epsilon_closure_in = regex.in->get_epsilon_closure();
        assert(epsilon_closure_in.size() == 3);
        assert(epsilon_closure_in[0] == regex.in.get());
        
        const auto epsilon_closure_out = regex.out->get_epsilon_closure();
        assert(epsilon_closure_out.size() == 1);
        assert(epsilon_closure_out[0] == regex.out.get());
    }

    cout << "OK.\n";
}

static void test_get_transitions_table()
{
    cout << __func__ << ":\n";

    NFA regex = NFA{'a'} | NFA{'b'};
    {
        GraphDumpVisitor visitor{"transitions table: regex='a|b'"};
        regex.accept(visitor);
        visitor.dump_graph("/tmp/transitions-table-union.dot");
    }
    {
        TransitionsTableVisitor visitor;
        regex.accept(visitor);

        TransitionsTable transitions_table = visitor.get_transitions_table();
        cout << transitions_table << '\n';

        assert(transitions_table.table.size() == 6);
        assert(transitions_table.epsilon_closures.size() == transitions_table.table.size());
        // TODO for loop to check that keys from table an epsilon closures should be the same.
    }
    cout << "OK.\n";
}

int main()
{
    // NFA Building Blocks Tests
    test_fundamental_machines();
    test_operator_concat();
    test_operator_union();
    test_operator_kleene_naive();
    test_complex_precedence();
    test_sugar_plus_naive();
    test_sugar_question_mark_naive();
    test_sugar_digit_char_class_naive();
    test_sugar_char_range_naive();
    test_optimizations_operator_kleene();
    test_optimizations_operator_plus();
    test_optimizations_operator_question_mark();
    test_optimizations_operator_char_range();

    // NFA Table Generation Tests
    test_epsilon_closure();
    test_get_transitions_table();

    return 0;
}