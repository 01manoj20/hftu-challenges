// Workload generator for Challenge 02: Multi-Symbol Order Book
// Shared between C++ and Rust private benchmarks.
// Produces identical workloads given the same seed.

#include "workload_gen.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <queue>
#include <random>
#include <unordered_map>
#include <vector>

namespace {

constexpr int NUM_SYMBOLS = 200;
constexpr int NUM_HOT = 50;
constexpr double HOT_PCT = 0.9;
constexpr size_t SETUP_ORDERS = 500'000;
constexpr size_t TIMED_OPS = 100'000;
constexpr int MAX_OUR_ORDERS = 50;
constexpr int OUR_ORDER_LATENCY = 10;
constexpr int64_t MAX_PRICE = 100'000;
constexpr int64_t TICK_SIZE = 100;

struct SymbolState {
    int64_t best_bid = 0;
    int64_t best_ask = 0;
    int64_t mid = 0;
};

SymbolState sym_state[NUM_SYMBOLS];

int64_t symbol_mid_price(int sym) {
    return 1000 + static_cast<int64_t>(sym) * (MAX_PRICE - 2000) / NUM_SYMBOLS;
}

int64_t snap_to_tick(int64_t price) {
    int64_t snapped = (price / TICK_SIZE) * TICK_SIZE;
    return snapped > 0 ? snapped : TICK_SIZE;
}

uint16_t pick_symbol(std::mt19937_64& gen) {
    std::uniform_real_distribution<double> pct(0.0, 1.0);
    if (pct(gen) < HOT_PCT)
        return static_cast<uint16_t>(std::uniform_int_distribution<int>(0, NUM_HOT - 1)(gen));
    return static_cast<uint16_t>(std::uniform_int_distribution<int>(NUM_HOT, NUM_SYMBOLS - 1)(gen));
}

int64_t pick_price(std::mt19937_64& gen, uint16_t symbol, int side) {
    auto& ss = sym_state[symbol];
    int64_t mid = ss.mid ? ss.mid : symbol_mid_price(symbol);

    std::uniform_real_distribution<double> pct(0.0, 1.0);
    double r = pct(gen);

    int64_t raw;
    if (r < 0.60) {
        int64_t base = (side == 0) ? (ss.best_bid ? ss.best_bid : mid - 5 * TICK_SIZE)
                                   : (ss.best_ask ? ss.best_ask : mid + 5 * TICK_SIZE);
        int64_t offset = std::uniform_int_distribution<int64_t>(-2, 2)(gen) * TICK_SIZE;
        raw = base + offset;
    } else if (r < 0.90) {
        int64_t offset = std::uniform_int_distribution<int64_t>(-20, 20)(gen) * TICK_SIZE;
        raw = mid + offset;
    } else {
        if (pct(gen) < 0.5) {
            int64_t offset = std::uniform_int_distribution<int64_t>(-100, 100)(gen) * TICK_SIZE;
            raw = mid + offset;
        } else {
            raw = std::uniform_int_distribution<int64_t>(1, MAX_PRICE)(gen);
        }
    }
    raw = std::max(TICK_SIZE, std::min(MAX_PRICE, raw));
    return snap_to_tick(raw);
}

void update_sym_state(uint16_t symbol, int side, int64_t price) {
    auto& ss = sym_state[symbol];
    if (side == 0) {
        if (price > ss.best_bid) ss.best_bid = price;
    } else {
        if (ss.best_ask == 0 || price < ss.best_ask) ss.best_ask = price;
    }
    if (ss.best_bid && ss.best_ask)
        ss.mid = (ss.best_bid + ss.best_ask) / 2;
}

// Reference book for expected_bids/expected_asks
struct RefBook {
    std::map<int64_t, int64_t> bids, asks; // price -> total_qty
};

void ref_apply(RefBook* books, std::unordered_map<uint64_t, std::tuple<uint16_t, int8_t, int64_t, int64_t>>& orders,
               const WgOperation& op) {
    if (op.type == WG_ADD) {
        orders[op.id] = {op.symbol, op.side, op.price, op.qty};
        auto& levels = (op.side == 0) ? books[op.symbol].bids : books[op.symbol].asks;
        levels[op.price] += op.qty;
    } else if (op.type == WG_CANCEL) {
        auto it = orders.find(op.id);
        if (it == orders.end()) return;
        auto [sym, side, price, qty] = it->second;
        orders.erase(it);
        auto& levels = (side == 0) ? books[sym].bids : books[sym].asks;
        auto lit = levels.find(price);
        if (lit != levels.end()) {
            lit->second -= qty;
            if (lit->second <= 0) levels.erase(lit);
        }
    } else if (op.type == WG_MODIFY) {
        auto it = orders.find(op.id);
        if (it == orders.end()) return;
        auto& [sym, side, price, qty] = it->second;
        auto& levels = (side == 0) ? books[sym].bids : books[sym].asks;
        auto lit = levels.find(price);
        if (lit != levels.end()) lit->second += (op.qty - qty);
        qty = op.qty;
    }
}

} // anonymous namespace

extern "C" {

WgWorkload* wg_generate_workload(uint64_t seed) {
    std::mt19937_64 gen(seed);

    auto* wl = new WgWorkload{};
    std::vector<WgOperation> setup;
    std::vector<WgOperation> timed;
    setup.reserve(SETUP_ORDERS);

    uint64_t next_eid = 1;
    uint64_t next_our_id = 1;

    struct ActiveOrder { uint64_t eid; uint16_t symbol; int8_t side; int64_t price; int64_t qty; };
    std::vector<ActiveOrder> active;
    active.reserve(SETUP_ORDERS + TIMED_OPS);

    struct OurOrder { uint64_t our_id, exchange_id; uint16_t symbol; int8_t side; int64_t price, qty; };
    std::vector<OurOrder> our_active;

    struct PendingEvent {
        size_t trigger_at;
        WgOperation op;
        bool operator>(const PendingEvent& o) const { return trigger_at > o.trigger_at; }
    };
    std::priority_queue<PendingEvent, std::vector<PendingEvent>, std::greater<>> pending;

    std::uniform_int_distribution<int64_t> qty_dist(1, 10'000);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> op_dist(0, 99);

    // Reference book for state verification
    RefBook ref_books[NUM_SYMBOLS];
    std::unordered_map<uint64_t, std::tuple<uint16_t, int8_t, int64_t, int64_t>> ref_orders;

    // Initialize symbol state
    std::memset(sym_state, 0, sizeof(sym_state));
    for (int s = 0; s < NUM_SYMBOLS; ++s)
        sym_state[s].mid = snap_to_tick(symbol_mid_price(s));

    // Setup phase
    for (size_t i = 0; i < SETUP_ORDERS; ++i) {
        uint16_t sym = pick_symbol(gen);
        int8_t side = static_cast<int8_t>(side_dist(gen));
        int64_t price = pick_price(gen, sym, side);
        int64_t qty = qty_dist(gen);
        uint64_t eid = next_eid++;
        update_sym_state(sym, side, price);
        WgOperation op = {WG_ADD, eid, sym, side, price, qty, 0};
        ref_apply(ref_books, ref_orders, op);
        setup.push_back(op);
        active.push_back({eid, sym, side, price, qty});
    }
    wl->venue_start_eid = next_eid;

    // Timed phase
    timed.reserve(TIMED_OPS * 2);
    size_t op_idx = 0;

    auto drain_pending = [&]() {
        while (!pending.empty() && pending.top().trigger_at <= op_idx) {
            auto op = pending.top().op;
            ref_apply(ref_books, ref_orders, op);
            timed.push_back(op);
            op_idx++;
            pending.pop();
        }
    };

    for (size_t i = 0; i < TIMED_OPS; ++i) {
        drain_pending();
        int r = op_dist(gen);

        if (r < 35 || active.size() < 100) {
            uint16_t sym = pick_symbol(gen);
            int8_t side = static_cast<int8_t>(side_dist(gen));
            int64_t price = pick_price(gen, sym, side);
            int64_t qty = qty_dist(gen);
            uint64_t eid = next_eid++;
            update_sym_state(sym, side, price);
            WgOperation op = {WG_ADD, eid, sym, side, price, qty, 0};
            ref_apply(ref_books, ref_orders, op);
            timed.push_back(op);
            active.push_back({eid, sym, side, price, qty});
        } else if (r < 55 && active.size() > 1000) {
            std::uniform_int_distribution<size_t> idx(0, active.size() - 1);
            size_t ci = idx(gen);
            WgOperation op = {WG_CANCEL, active[ci].eid, 0, 0, 0, 0, 0};
            ref_apply(ref_books, ref_orders, op);
            timed.push_back(op);
            active[ci] = active.back(); active.pop_back();
        } else if (r < 65 && active.size() > 100) {
            std::uniform_int_distribution<size_t> idx(0, active.size() - 1);
            size_t mi = idx(gen);
            auto& ao = active[mi];
            int64_t new_qty = std::max(int64_t(1), ao.qty * std::uniform_int_distribution<int64_t>(30, 90)(gen) / 100);
            ao.qty = new_qty;
            WgOperation op = {WG_MODIFY, ao.eid, 0, 0, 0, new_qty, 0};
            ref_apply(ref_books, ref_orders, op);
            timed.push_back(op);
        } else if (r < 70 && static_cast<int>(our_active.size()) < MAX_OUR_ORDERS) {
            uint16_t sym = pick_symbol(gen);
            int8_t side = static_cast<int8_t>(side_dist(gen));
            int64_t price = pick_price(gen, sym, side);
            int64_t qty = qty_dist(gen);
            uint64_t oid = next_our_id++;
            uint64_t eid = next_eid++;
            timed.push_back({WG_SEND_OUR, oid, sym, side, price, qty, 0});
            int delay = std::uniform_int_distribution<int>(3, OUR_ORDER_LATENCY)(gen);
            WgOperation add_op = {WG_ADD, eid, sym, side, price, qty, 0};
            pending.push({op_idx + static_cast<size_t>(delay) + 1, add_op});
            our_active.push_back({oid, eid, sym, side, price, qty});
            active.push_back({eid, sym, side, price, qty});
        } else if (r < 73 && !our_active.empty()) {
            std::uniform_int_distribution<size_t> idx(0, our_active.size() - 1);
            size_t mi = idx(gen);
            auto& oo = our_active[mi];
            if (std::uniform_int_distribution<int>(0, 99)(gen) < 60) {
                int64_t new_qty = std::max(int64_t(1), oo.qty * std::uniform_int_distribution<int64_t>(30, 90)(gen) / 100);
                timed.push_back({WG_MODIFY_OUR, oo.our_id, 0, 0, oo.price, new_qty, 0});
                int delay = std::uniform_int_distribution<int>(3, OUR_ORDER_LATENCY)(gen);
                WgOperation mod_op = {WG_MODIFY, oo.exchange_id, 0, 0, 0, new_qty, 0};
                pending.push({op_idx + static_cast<size_t>(delay) + 1, mod_op});
                oo.qty = new_qty;
                for (auto& a : active) { if (a.eid == oo.exchange_id) { a.qty = new_qty; break; } }
            } else {
                int64_t new_price = pick_price(gen, oo.symbol, oo.side);
                uint64_t old_eid = oo.exchange_id;
                uint64_t new_eid = next_eid++;
                timed.push_back({WG_MODIFY_OUR, oo.our_id, 0, 0, new_price, oo.qty, 0});
                int delay = std::uniform_int_distribution<int>(3, OUR_ORDER_LATENCY)(gen);
                size_t when = op_idx + static_cast<size_t>(delay) + 1;
                pending.push({when, {WG_CANCEL, old_eid, 0, 0, 0, 0, 0}});
                pending.push({when + 1, {WG_ADD, new_eid, oo.symbol, oo.side, new_price, oo.qty, 0}});
                for (auto& a : active) { if (a.eid == old_eid) { a.eid = new_eid; a.price = new_price; break; } }
                oo.exchange_id = new_eid;
                oo.price = new_price;
            }
        } else if (r < 75 && !our_active.empty()) {
            std::uniform_int_distribution<size_t> idx(0, our_active.size() - 1);
            size_t ci = idx(gen);
            auto& oo = our_active[ci];
            timed.push_back({WG_CANCEL_OUR, oo.our_id, 0, 0, 0, 0, 0});
            int delay = std::uniform_int_distribution<int>(3, OUR_ORDER_LATENCY)(gen);
            pending.push({op_idx + static_cast<size_t>(delay) + 1, {WG_CANCEL, oo.exchange_id, 0, 0, 0, 0, 0}});
            for (size_t j = 0; j < active.size(); ++j) {
                if (active[j].eid == oo.exchange_id) { active[j] = active.back(); active.pop_back(); break; }
            }
            our_active[ci] = our_active.back(); our_active.pop_back();
        } else if (r < 80) {
            uint16_t sym = pick_symbol(gen);
            timed.push_back({WG_BEST_BID, 0, sym, 0, 0, 0, 0});
        } else if (r < 85) {
            uint16_t sym = pick_symbol(gen);
            timed.push_back({WG_BEST_ASK, 0, sym, 0, 0, 0, 0});
        } else if (r < 90) {
            uint16_t sym = pick_symbol(gen);
            int8_t side = static_cast<int8_t>(side_dist(gen));
            int n = std::uniform_int_distribution<int>(1, 5)(gen);
            timed.push_back({WG_TOP_LEVELS, 0, sym, side, 0, 0, n});
        } else if (r < 95) {
            uint16_t sym = pick_symbol(gen);
            int8_t side = static_cast<int8_t>(side_dist(gen));
            int64_t depth = std::uniform_int_distribution<int64_t>(5, 50)(gen);
            timed.push_back({WG_VOLUME_NEAR_BEST, 0, sym, side, 0, 0, depth});
        } else {
            if (!our_active.empty()) {
                std::uniform_int_distribution<size_t> idx(0, our_active.size() - 1);
                timed.push_back({WG_QUEUE_POSITION, our_active[idx(gen)].our_id, 0, 0, 0, 0, 0});
            } else {
                uint16_t sym = pick_symbol(gen);
                timed.push_back({WG_BEST_BID, 0, sym, 0, 0, 0, 0});
            }
        }
        op_idx = timed.size();
    }

    // Drain remaining pending
    while (!pending.empty()) {
        auto op = pending.top().op;
        ref_apply(ref_books, ref_orders, op);
        timed.push_back(op);
        pending.pop();
    }

    // Copy to output
    wl->num_setup = static_cast<uint32_t>(setup.size());
    wl->setup = new WgOperation[setup.size()];
    std::memcpy(wl->setup, setup.data(), setup.size() * sizeof(WgOperation));

    wl->num_timed = static_cast<uint32_t>(timed.size());
    wl->timed = new WgOperation[timed.size()];
    std::memcpy(wl->timed, timed.data(), timed.size() * sizeof(WgOperation));

    // Expected state
    for (int s = 0; s < NUM_SYMBOLS; ++s) {
        wl->expected_bids[s] = ref_books[s].bids.empty() ? 0 : ref_books[s].bids.rbegin()->first;
        wl->expected_asks[s] = ref_books[s].asks.empty() ? 0 : ref_books[s].asks.begin()->first;
    }

    return wl;
}

void wg_free_workload(WgWorkload* wl) {
    if (!wl) return;
    delete[] wl->setup;
    delete[] wl->timed;
    delete wl;
}

} // extern "C"
