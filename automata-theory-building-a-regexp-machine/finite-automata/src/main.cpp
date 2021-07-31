#include <iostream>
#include <memory>
#include <optional>

#include "fa/state.h"

using namespace std;
using namespace fa;

int main()
{
    shared_ptr<State> s1 = make_shared<State>(false);
    shared_ptr<State> s2 = make_shared<State>(true);

    s1->add_transition_for_symbol("a", s2);
    cout << *s1 << '\n';
    return 0;
}