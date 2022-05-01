const std = @import("std");

const Allocator = std.mem.Allocator;
const StringHashMap = std.StringHashMap;
const AutoHashMap = std.AutoHashMap;
const ArrayList = std.ArrayList;
const TailQueue = std.TailQueue;
const assert = std.debug.assert;

const nfa = struct {
    pub const SymbolKind = enum {
        epsilon,
        char,
    };
    pub const Symbol = u8;
    // pub const Symbol = union(SymbolKind) {
    //     epsilon,
    //     char: u8,
    // };
    pub const State = struct {
        accepting: bool,
        id: StateId,

        pub fn loadTable(self: *const @This(), table: *Table, visited_states: *AutoHashMap(StateId, void)) void {
            if (visited_states.get(self.id) == null)
                return;

            visited_states.put(self.id, void) catch @panic("out of memory");
            table.
            // for each transition, visit it
            // for each epsilon transition, visit it
        }
    };
    pub const StateId = usize;

    pub const SymbolTransitionKey = struct {
        from: StateId,
        symbol: Symbol,
    };
    pub const EpsilonTransitionKey = struct {
        from: StateId,
    };
    pub const TransitionValue = ArrayList(StateId);

    pub const NFA = struct {
        in: StateId,
        out: StateId,
    };

    pub const Graph = struct {
        allocator: *Allocator,
        states: ArrayList(State),
        transitions: AutoHashMap(SymbolTransitionKey, TransitionValue),
        e_transitions: AutoHashMap(EpsilonTransitionKey, TransitionValue),
        symbols: AutoHashMap(Symbol, void),
        nfa: NFA,

        pub fn init(allocator: *Allocator) @This() {
            return @This() {
                .allocator = allocator,
                .states = ArrayList(State).init(allocator),
                .transitions = AutoHashMap(SymbolTransitionKey, TransitionValue).init(allocator),
                .e_transitions = AutoHashMap(EpsilonTransitionKey, TransitionValue).init(allocator),
                .symbols = AutoHashMap(Symbol, void).init(allocator),
                .nfa = undefined,
            };
        }

        pub fn deinit(self: *@This()) void {
            self.symbols.deinit();
            self.e_transitions.deinit();
            self.transitions.deinit();
            self.states.deinit();
        }

        pub fn addState(self: *@This(), accepting: bool) usize {
            var new_state = State {
                .accepting = accepting,
                .id = self.states.items.len,
            };
            self.states.append(new_state) catch @panic("out of memory");
            return self.states.items.len - 1;
        }

        pub inline fn getState(self: *@This(), state_id: usize) *State {
            assert(state_id < self.states.items.len);
            return &self.states.items[state_id];
        }

        pub inline fn getInState(self: *@This()) *State {
            return self.getState(self.nfa.in);
        }

        pub inline fn getStateConst(self: *const @This(), state_id: usize) *const State {
            assert(state_id < self.states.items.len);
            return &self.states.items[state_id];
        }

        pub inline fn getInStateConst(self: *const @This()) *const State {
            return self.getStateConst(self.nfa.in);
        }

        pub fn addSymbolTransition(self: *@This(), from: usize, symbol: Symbol, to: usize) void {
            const key = SymbolTransitionKey {
                .from = from,
                .symbol = symbol
            };
            var result = self.transitions.getOrPut(key) catch @panic("out of memory");
            if (result.found_existing) {
                result.entry.value.append(to) catch @panic("out of memory");
            } else {
                result.entry.value = TransitionValue.init(self.allocator);
                result.entry.value.append(to) catch @panic("out of memory");
            }
            self.symbols.put(symbol, {}) catch @panic("out of memory");
        }

        pub fn addEpsilonTransition(self: *@This(), from: usize, to: usize) void {
            const key = EpsilonTransitionKey{ .from = from };
            var result = self.e_transitions.getOrPut(key) catch @panic("out of memory");
            if (result.found_existing) {
                result.entry.value.append(to) catch @panic("out of memory");
            } else {
                result.entry.value = TransitionValue.init(self.allocator);
                result.entry.value.append(to) catch @panic("out of memory");
            }
        }

        pub fn char(self: *@This(), c: Symbol) NFA {
            const s0 = self.addState(false);
            const s1 = self.addState(true);

            self.addSymbolTransition(s0, c, s1);

            return NFA {
                .in = s0,
                .out = s1,
            };
        }

        pub fn epsilon(self: *@This()) NFA {
            const s0: usize = self.addState(false);
            const s1: usize = self.addState(true);

            self.addEpsilonTransition(s0, s1);

            return NFA {
                .in = s0,
                .out = s1,
            };
        }

        pub fn concat(self: *@This(), a: NFA, b: NFA) NFA {
            self.addEpsilonTransition(a.out, b.in);
            self.getState(a.out).accepting = false;
            self.getState(b.out).accepting = true;
            return NFA {
                .in = a.in,
                .out = b.out,
            };
        }

        pub fn disjunction(self: *@This(), a: NFA, b: NFA) NFA {
            const start: usize = self.addState(false);
            const end: usize = self.addState(true);

            self.addEpsilonTransition(start, a.in);
            self.addEpsilonTransition(start, b.in);
            self.addEpsilonTransition(a.out, end);
            self.addEpsilonTransition(b.out, end);

            self.getState(a.out).accepting = false;
            self.getState(b.out).accepting = false;

            return NFA {
                .in = start,
                .out = end,
            };
        }

        pub fn zeroOrMore(self: *@This(), a: NFA) NFA {
            // zero...
            self.addEpsilonTransition(a.in, a.out);
            // or more...
            self.addEpsilonTransition(a.out, a.in);
            return a;
        }

        pub fn oneOrMore(self: *@This(), a: NFA) NFA {
            // one: ok already
            // or more...
            self.addEpsilonTransition(a.out, a.in);
            return a;
        }

        pub fn optional(self: *@This(), a: NFA) NFA {
            self.addEpsilonTransition(a.in, a.out);
            return a;
        }

        pub fn range(self: *@This(), from: Symbol, to: Symbol) NFA {
            assert(from <= to);

            // 2 states: [A, B]; 1 transition: A -(from)-> B
            const resulting = self.char(from);

            // start from the next transition
            var i: Symbol = from + 1;

            // loop over inclusive range
            while (i <= to) : (i += 1) {
                self.addSymbolTransition(resulting.in, i, resulting.out);
            }

            return resulting;
        }

        // TODO Avaliar se faz sentido
        /// Returns all (non-epsilon) symbol transitions for a given state.
        /// The caller owns the returned memory
        pub fn getTransitionsForState(self: *const @This(), state: usize) AutoHashMap(Symbol, usize) {
            var transitions = AutoHashMap(Symbol, usize).init(self.allocator);
            errdefer transitions.deinit();

            var iter = self.transitions.iterator();
            while (iter.next()) |transition| {
                if (transition.key.from != state) continue;

                assert(transition.value.items.len < std.math.maxInt(u32));

                const new_capacity: u32 = transitions.count() + @truncate(u32, transition.value.items.len);
                transitions.ensureCapacity(new_capacity) catch @panic("out of memory");
                for (transition.value.items) |t| {
                    transitions.putAssumeCapacityNoClobber(transition.key.symbol, t);
                }
            }
            return transitions;
        }

        // TODO Avaliar se faz sentido
        /// Returns the epsilon closure for a given state.
        /// The caller owns the returned memory
        pub fn getEpsilonClosure(self: *const @This(), state: usize) ArrayList(usize) {
            var e_closure = ArrayList(usize).init(self.allocator);
            errdefer e_closure.deinit();

            e_closure.append(state) catch @panic("out of memory");
            var iter = self.e_transitions.iterator();
            while (iter.next()) |e_transition| {
                if (e_transition.key.from != state) continue;

                // TODO is there a better way to append an arraylist into another?
                e_closure.ensureCapacity(e_closure.items.len + e_transition.value.items.len) catch @panic("out of memory");
                for (e_transition.value.items) |t| {
                    e_closure.appendAssumeCapacity(t);
                }
            }
            return e_closure;
        }
    };
    pub const Table = struct {
        entries: AutoHashMap(StateId, EntryValue),

        pub const EntryValue = struct {
            transitions: AutoHashMap(Symbol, StateId),

            pub fn init(allocator: *Allocator) @This() {
                return .{.transitions = AutoHashMap(Symbol, StateId).init(allocator)};
            }
        };
        
        pub fn init(allocator: *Allocator) @This() {
            return .{.entries = AutoHashMap(StateId, EntryValue).init(allocator)};
        }

        pub fn deinit(self: *@This()) void {
            self.entries.deinit();
        }

        pub fn addTransition(self: *@This(), from: StateId, symbol: Symbol, to: StateId) void {
            // TODO
            unreachable;
        }

        pub fn load(self: *@This(), g: *Graph) void {
            var visited_states = AutoHashMap(StateId, void).init(g.allocator);
            g.getInStateConst().loadTable(self, &visited_states);
        }
    };
};
pub const dfa = struct {
    // TODO Avaliar esta Tabela e sua contrução
    pub const State = struct {
        accepting: bool,
    };
    pub const Transition = AutoHashMap(nfa.Symbol, usize);
    pub const Table = struct {
        allocator: *Allocator,
        states: ArrayList(State),
        initial_state: usize,

        pub fn init(allocator: *Allocator) @This() {
            return @This() {
                .allocator = allocator,
                .states = ArrayList(State).init(allocator),
                .initial_state = undefined,
            };
        }

        pub fn deinit(self: *@This()) void {
            self.states.deinit();
        }

        pub fn load(self: *@This(), g: *const nfa.Graph) void {
            const initial_nfa_state: usize = g.nfa.in;
            const initial_nfa_state_e_closure: ArrayList(usize) = g.getEpsilonClosure(initial_nfa_state);
            const initial_nfa_state_transitions: AutoHashMap(nfa.Symbol, usize) = g.getTransitionsForState(initial_nfa_state);

            self.initial_state = self.addState(g, initial_nfa_state_e_closure.items);
            var initial_dfa_state: *State = self.getState(self.initial_state);

            const StatesQueue = TailQueue(usize);
            var states_queue = StatesQueue{};
            var initial_state_node = StatesQueue.Node{ .data = self.initial_state };
            states_queue.append(&initial_state_node);
            while (states_queue.len > 0) {
                if (states_queue.popFirst()) |state| {
                    // var iter = 
                }
            }
        }

        pub fn addState(self: *@This(), g: *const nfa.Graph, nfa_states: []usize) usize {
            var accepting = false;
            for (nfa_states) |state_id| {
                if (g.getStateConst(state_id).accepting) {
                    accepting = true;
                    break;
                }
            }
            self.states.append(State{ .accepting = accepting }) catch @panic("out of memory");
            return self.states.items.len - 1;
        }

        pub inline fn getState(self: *@This(), index: usize) *State {
            return &self.states.items[index];
        }
    };
};

const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;

test "nfa: basics" {
    const s: nfa.Symbol = 'a';

    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const allocator: *Allocator = &arena.allocator;
    var g: nfa.Graph = nfa.Graph.init(allocator);

    const s0: usize = g.addState(false);
    expectEqual(s0, 0);
    expect(!g.getState(s0).accepting);
    expectEqual(g.getState(s0).id, s0);

    const s1: usize = g.addState(true);
    expectEqual(s1, 1);
    expect(g.getState(s1).accepting);
    expectEqual(g.getState(s1).id, s1);

    g.addSymbolTransition(s0, 'a', s1);
    g.addEpsilonTransition(s1, s0);

    expectEqual(@as(usize, 2), g.states.items.len);
    expectEqual(@as(usize, 1), g.transitions.count());
    expectEqual(@as(usize, 1), g.e_transitions.count());
}

test "nfa: elementary machines: char" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const allocator: *Allocator = &arena.allocator;

    var g: nfa.Graph = nfa.Graph.init(allocator);
    defer g.deinit();

    const s: nfa.Symbol = 'a';
    const a = g.char(s);
    
    // state expectations
    expectEqual(@as(usize, 2), g.states.items.len);
    expect(!g.getState(a.in).accepting);
    expect(g.getState(a.out).accepting);

    // symbol transitions expectations
    expectEqual(@as(usize, 1), g.transitions.count());
    var transitions_iter = g.transitions.iterator();
    const transition = transitions_iter.next();
    expect(transition != null);
    expectEqual(a.in, transition.?.key.from);
    expectEqual(s, transition.?.key.symbol);
    expectEqual(@as(usize, 1), transition.?.value.items.len);
    expectEqual(a.out, transition.?.value.items[0]);
    expectEqual(@as(u32, 1), g.symbols.count());
    expect(g.symbols.contains('a'));

    // epsilon transitions expectations
    expectEqual(@as(usize, 0), g.e_transitions.count());
}

test "nfa: elementary machines: epsilon" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const allocator: *Allocator = &arena.allocator;

    var g: nfa.Graph = nfa.Graph.init(allocator);
    defer g.deinit();

    const e = g.epsilon();

    // state expectations
    expectEqual(@as(usize, 2), g.states.items.len);
    expect(!g.getState(e.in).accepting);
    expect(g.getState(e.out).accepting);

    // symbol transitions expectations
    expectEqual(@as(usize, 0), g.transitions.count());
    expectEqual(@as(u32, 0), g.symbols.count());

    // epsilon transitions expectations
    expectEqual(@as(usize, 1), g.e_transitions.count());
    var e_transitions_iter = g.e_transitions.iterator();
    const e_transition = e_transitions_iter.next();
    expect(e_transition != null);
    expectEqual(e.in, e_transition.?.key.from);
    expectEqual(@as(usize, 1), e_transition.?.value.items.len);
    expectEqual(e.out, e_transition.?.value.items[0]);
}

test "nfa: elementary operators: concat" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const allocator: *Allocator = &arena.allocator;

    var g: nfa.Graph = nfa.Graph.init(allocator);
    defer g.deinit();

    const a = g.char('a');
    const b = g.char('b');
    const ab = g.concat(a, b);

    // states expectations
    expectEqual(@as(usize, 4), g.states.items.len);
    expect(!g.getState(a.in).accepting);
    expect(!g.getState(a.out).accepting);
    expect(!g.getState(b.in).accepting);
    expect(g.getState(b.out).accepting);
    expect(!g.getState(ab.in).accepting);
    expect(g.getState(ab.out).accepting);
    expectEqual(a.in, ab.in);
    expectEqual(b.out, ab.out);

    // symbol transitions expectations
    expectEqual(@as(usize, 2), g.transitions.count());
    expectEqual(@as(u32, 2), g.symbols.count());
    expect(g.symbols.contains('a'));
    expect(g.symbols.contains('b'));

    // epsilon transitions expectations
    expectEqual(@as(usize, 1), g.e_transitions.count());
    var e_transitions_iter = g.e_transitions.iterator();
    const e_transition = e_transitions_iter.next();
    expect(e_transition != null);
    expectEqual(a.out, e_transition.?.key.from);
    expectEqual(@as(usize, 1), e_transition.?.value.items.len);
    expectEqual(b.in, e_transition.?.value.items[0]);
}

test "nfa: elementary operators: disjunction (op: '|')" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const allocator: *Allocator = &arena.allocator;

    var g: nfa.Graph = nfa.Graph.init(allocator);
    defer g.deinit();

    const a = g.char('a');
    const b = g.char('b');
    const a_or_b = g.disjunction(a, b);

    // states expectations
    expectEqual(@as(usize, 6), g.states.items.len);
    expect(!g.getState(a.in).accepting);
    expect(!g.getState(a.out).accepting);
    expect(!g.getState(b.in).accepting);
    expect(!g.getState(b.out).accepting);
    expect(!g.getState(a_or_b.in).accepting);
    expect(g.getState(a_or_b.out).accepting);

    // symbol transitions expectations
    expectEqual(@as(usize, 2), g.transitions.count());
    expectEqual(@as(u32, 2), g.symbols.count());
    expect(g.symbols.contains('a'));
    expect(g.symbols.contains('b'));

    // epsilon transitions expectations
    // 1: key=(a_or_b.in), value=[a.in, b.in]
    // 2: key=(a.out), value=[a_or_b.out]
    // 3: key=(b.out), value=[a_or_b.out]
    expectEqual(@as(usize, 3), g.e_transitions.count());
}

test "nfa: elementary operators: zeroOrMore (kleene star) (op: '*')" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const allocator: *Allocator = &arena.allocator;

    var g: nfa.Graph = nfa.Graph.init(allocator);
    defer g.deinit();

    const a = g.char('a');
    const m = g.zeroOrMore(a);

    // states expectations
    expectEqual(@as(usize, 2), g.states.items.len);
    expect(!g.getState(m.in).accepting);
    expect(g.getState(m.out).accepting);
    expectEqual(a.in, m.in);
    expectEqual(a.out, m.out);

    // symbol transitions expectations
    expectEqual(@as(usize, 1), g.transitions.count());

    // epsilon transitions expectations
    expectEqual(@as(usize, 2), g.e_transitions.count());
    // TODO improve tests by checking the epsilon transitions:
    //      1: from m.in to m.out
    //      2: from m.out to m.in
}

test "nfa: syntatic sugar: oneOrMore (op: '+')" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const allocator: *Allocator = &arena.allocator;

    var g: nfa.Graph = nfa.Graph.init(allocator);
    defer g.deinit();

    const a = g.char('a');
    const m = g.oneOrMore(a);

    // states expectations
    expectEqual(@as(usize, 2), g.states.items.len);
    expect(!g.getState(m.in).accepting);
    expect(g.getState(m.out).accepting);
    expectEqual(a.in, m.in);
    expectEqual(a.out, m.out);

    // symbol transitions expectations
    expectEqual(@as(usize, 1), g.transitions.count());

    // epsilon transitions expectations
    expectEqual(@as(usize, 1), g.e_transitions.count());
    // TODO improve tests by checking the epsilon transition from m.out to m.in
}

test "nfa: syntatic sugar: optional (op: '?')" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const allocator: *Allocator = &arena.allocator;

    var g: nfa.Graph = nfa.Graph.init(allocator);
    defer g.deinit();

    const a = g.char('a');
    const m = g.optional(a);

    // states expectations
    expectEqual(@as(usize, 2), g.states.items.len);
    expect(!g.getState(m.in).accepting);
    expect(g.getState(m.out).accepting);
    expectEqual(a.in, m.in);
    expectEqual(a.out, m.out);

    // symbol transitions expectations
    expectEqual(@as(usize, 1), g.transitions.count());

    // epsilon transitions expectations
    expectEqual(@as(usize, 1), g.e_transitions.count());
    // TODO improve tests by checking the epsilon transition from m.in to m.out
}

test "nfa: syntatic sugar: range ([X-Y])" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const allocator: *Allocator = &arena.allocator;

    var g: nfa.Graph = nfa.Graph.init(allocator);
    defer g.deinit();

    const digit = g.range('0', '9');

    // states expectations
    expectEqual(@as(usize, 2), g.states.items.len);
    expect(!g.getState(digit.in).accepting);
    expect(g.getState(digit.out).accepting);

    // symbol transitions expectations
    expectEqual(@as(usize, 10), g.transitions.count());

    // epsilon transitions expectations
    expectEqual(@as(usize, 0), g.e_transitions.count());
}

test "nfa: table: load" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const allocator: *Allocator = &arena.allocator;

    var g: nfa.Graph = nfa.Graph.init(allocator);
    defer g.deinit();

    g.nfa = g.disjunction(g.char('a'), g.char('b'));

    // TODO Avaliar este teste e sua implementacao
    var table = nfa.Table.init(allocator);
    defer table.deinit();

    table.load(&g);
}

test "dfa: table: load" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const allocator: *Allocator = &arena.allocator;

    var g: nfa.Graph = nfa.Graph.init(allocator);
    defer g.deinit();

    g.nfa = g.disjunction(g.char('a'), g.char('b'));

    // TODO Avaliar este teste e sua implementacao

    var table: dfa.Table = dfa.Table.init(allocator);
    defer table.deinit();

    table.load(&g);
}