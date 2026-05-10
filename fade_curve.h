#ifndef FADE_CURVE_H
#define FADE_CURVE_H

#include <math.h>

/**
 * Exponential decay mapped to [0,1] progress: fast drop at t≈0, long tail near t≈1.
 *
 * @param t      Normalized position in the fade, typically 0 (start / full) → 1 (end / off).
 * @param decay  Steepness; larger values → snappier initial drop and longer low tail.
 * @return       Factor in [0, 1]; 1 at t=0, 0 at t=1 (for decay > 0).
 */
inline float fadeExponentialTailFactor(float t, float decay) {
  if (t <= 0.f) {
    return 1.f;
  }
  if (t >= 1.f) {
    return 0.f;
  }
  if (decay <= 0.f) {
    return 1.f - t;
  }
  const float expFull = expf(-decay);
  return (expf(-decay * t) - expFull) / (1.0f - expFull);
}

#endif
