#pragma once

static const char* SWITCH_CPU_PROFILES[] = {
    "Maximum Performance", "High Performance",   "Boost Performance",  "Stock Performance",
    "Powersaving Mode 1",  "Powersaving Mode 2", "Powersaving Mode 3",
};

static unsigned SWITCH_CPU_SPEEDS_VALUES[] = { 1785000000, 1581000000, 1224000000, 1020000000,
                                               918000000,  816000000,  714000000 };

// GPU + memory (EMC) clocks per profile, indexed the same way as the CPU table.
// Fast3D rendering of busy scenes (many fighters/effects + hi-res textures) is
// GPU- and bandwidth-bound, not CPU-bound, so the CPU-only overclock left the
// real bottleneck untouched. The top profiles force the official *docked*-tier
// GPU/EMC clocks even in handheld (these are within hardware spec — not a true
// overclock — just the higher clocks the SoC already runs when docked). All
// values are valid clkrst steps; the driver snaps to the nearest if not.
static unsigned SWITCH_GPU_SPEEDS_VALUES[] = { 768000000, 691200000, 614400000, 384000000,
                                               307200000, 230400000, 153600000 };

static unsigned SWITCH_EMC_SPEEDS_VALUES[] = { 1600000000, 1600000000, 1331200000, 1331200000,
                                               1065600000, 800000000,  665600000 };