const std = @import("std");

const Allocator = std.mem.Allocator;
const StringHashMap = std.StringHashMap;
const AutoHashMap = std.AutoHashMap;
const ArrayList = std.ArrayList;
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

    pub const StateLabelCap: usize = 10;
    pub const State = struct {
        accepting: bool,
        id: usize,
        label_buf: [StateLabelCap]u8,
        label: []u8,

        pub fn labelPrint(self: *@This(), comptime fmt: []const u8, args: anytype) std.fmt.BufPrintError!void {
            self.label = try std.fmt.bufPrint(&self.label_buf, fmt, args);
        }
    };

    pub const SymbolTransitionKey = struct {
        from: usize,
        symbol: Symbol,
    };
    pub const EpsilonTransitionKey = struct {
        from: usize,
    };
    pub const TransitionValue = ArrayList(usize);

    pub const NFA = struct {
        in: usize,
        out: usize,
    };

    pub const Graph = struct {
        allocator: *Allocator,
        states: ArrayList(State),
        transitions: AutoHashMap(SymbolTransitionKey, TransitionValue),
        e_transitions: AutoHashMap(EpsilonTransitionKey, TransitionValue),
        nfa: NFA,

        pub fn init(allocator: *Allocator) @This() {
            return @This() {
                .allocator = allocator,
                .states = ArrayList(State).init(allocator),
                .transitions = AutoHashMap(SymbolTransitionKey, TransitionValue).init(allocator),
                .e_transitions = AutoHashMap(EpsilonTransitionKey, TransitionValue).init(allocator),
                .nfa = NFA{.in = 0, .out = 0},
            };
        }

        pub fn deinit(self: *@This()) void {
            self.e_transitions.deinit();
            self.transitions.deinit();
            self.states.deinit();
        }

        pub fn addState(self: *@This(), accepting: bool) usize {
            var new_state = State {
                .accepting = accepting,
                .id = self.states.items.len,
                .label_buf = [_]u8{0} ** StateLabelCap,
                .label = undefined,
            };
            new_state.labelPrint("S{}", .{new_state.id}) catch unreachable;

            self.states.append(new_state) catch @panic("out of memory");
            return self.states.items.len - 1;
        }

        pub inline fn getState(self: *@This(), state_id: usize) *State {
            assert(state_id < self.states.items.len);
            return &self.states.items[state_id];
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
                result.entry.value = TransitionValue.initCapacity(self.allocator, 1) catch @panic("out of memory");
                result.entry.value.appendAssumeCapacity(to);
            }
        }

        pub fn addEpsilonTransition(self: *@This(), from: usize, to: usize) void {
            const key = EpsilonTransitionKey{ .from = from };
            var result = self.e_transitions.getOrPut(key) catch @panic("out of memory");
            if (result.found_existing) {
                result.entry.value.append(to) catch @panic("out of memory");
            } else {
                result.entry.value = TransitionValue.initCapacity(self.allocator, 1) catch @panic("out of memory");
                result.entry.value.appendAssumeCapacity(to);
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
    expectEqualStrings(g.getState(s0).label, "S0");

    const s1: usize = g.addState(true);
    expectEqual(s1, 1);
    expect(g.getState(s1).accepting);
    expectEqual(g.getState(s1).id, s1);
    expectEqualStrings(g.getState(s1).label, "S1");

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

    // epsilon transitions expectations
    expectEqual(@as(usize, 0), g.e_transitions.count());
}

test "nfa: elementary machines: epsilon" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const allocator: *Allocator = &arena.allocator;
    var g: nfa.Graph = nfa.Graph.init(allocator);

    const e = g.epsilon();

    // state expectations
    expectEqual(@as(usize, 2), g.states.items.len);
    expect(!g.getState(e.in).accepting);
    expect(g.getState(e.out).accepting);

    // symbol transitions expectations
    expectEqual(@as(usize, 0), g.transitions.count());

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
