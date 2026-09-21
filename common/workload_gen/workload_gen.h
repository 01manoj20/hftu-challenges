// C API for workload generation — shared between C++ and Rust benchmarks.
// Produces identical workloads to the C++ challenge-02 private benchmark.
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Must match Rust #[repr(C)] layout exactly
enum WgOpType : uint8_t {
    WG_ADD = 0,
    WG_CANCEL = 1,
    WG_MODIFY = 2,
    WG_SEND_OUR = 3,
    WG_MODIFY_OUR = 4,
    WG_CANCEL_OUR = 5,
    WG_BEST_BID = 6,
    WG_BEST_ASK = 7,
    WG_TOP_LEVELS = 8,
    WG_VOLUME_NEAR_BEST = 9,
    WG_QUEUE_POSITION = 10,
};

#pragma pack(push, 1)
struct WgOperation {
    uint8_t type;
    uint64_t id;
    uint16_t symbol;
    int8_t side;
    int64_t price;
    int64_t qty;
    int64_t aux;
};
#pragma pack(pop)

struct WgWorkload {
    struct WgOperation* setup;
    uint32_t num_setup;
    struct WgOperation* timed;
    uint32_t num_timed;
    uint64_t venue_start_eid;
    int64_t expected_bids[200];
    int64_t expected_asks[200];
};

// Generate a workload. Caller must free with wg_free_workload().
struct WgWorkload* wg_generate_workload(uint64_t seed);

// Free a workload.
void wg_free_workload(struct WgWorkload* wl);

#ifdef __cplusplus
}
#endif
