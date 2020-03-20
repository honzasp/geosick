# This script tests the formula for estimating
#   P(dist(x1, x2) < R_infect)
# given x1, x2 uniformly sampled from circles of radius R1, R2 at distance D.
#
# We use the formula
#   (A_infect * A_isect) / (A_1 * A_2)
# where A_1, A_2, A_infect are areas of circles with radius R1, R2, R_infect, and A_isect
# is the area of the intersection of two circles with radius R1, R2 at distance D.
#
# We obtain ground truth values using Monte Carlo simulation and compare them with the
# values provided by the formula.
#
# Run this script using `python3 -m geosick.formula_test`
from dataclasses import dataclass
import numpy as np

from .meet import circle_isect_area

@dataclass
class TestCase:
    r1: float
    r2: float
    dist: float
    ri: float

def sample_circle(rng, size):
    phi = rng.uniform(0, 2*np.pi, size)
    r = np.sqrt(rng.uniform(0, 1, size))
    x = np.sin(phi) * r
    y = np.cos(phi) * r
    return np.array([x, y])

def estimate_mc(test: TestCase):
    rng = np.random.default_rng(42)
    sample_size = 500*1000
    points_1 = sample_circle(rng, sample_size) * test.r1
    points_2 = sample_circle(rng, sample_size) * test.r2
    offset = np.array((test.dist, 0)).reshape(2, 1)
    dists_sq = np.power(points_1 - points_2 + offset, 2).sum(axis=0)
    return (dists_sq <= test.ri**2).astype(np.float).mean()

def estimate_formula(test: TestCase):
    area_1 = np.pi * test.r1**2
    area_2 = np.pi * test.r2**2
    area_isect = circle_isect_area(test.r1, test.r2, test.dist)
    area_infect = np.pi * test.ri**2
    return (area_infect*area_isect) / (area_1*area_2)

TEST_CASES = [
    TestCase(r1, r2, dist, 2.0)
    for (r1, r2) in [(3.0,3.0), (3.0,5.0), (5.0,5.0), (5.0,10.0), (10.0,10.0)]
    for dist in [0.0, 0.1*r2, 0.2*r2, 0.5*r2, 0.8*r2, 0.9*r2]
]

if __name__ == "__main__":
    print(f"{'r1':>4s} {'r2':>4s} {'dist':>4s} {'ri':>4s}   "
          f"{'mc':>5} {'form':>5s} {'abse':>6s} {'rele':>5s}")
    for test in TEST_CASES:
        mc = estimate_mc(test)
        formula = estimate_formula(test)
        print(f"{test.r1:4g} {test.r2:4g} {test.dist:4g} {test.ri:4g}   "
            f"{mc:5.3f} {formula:5.3f} {formula-mc:+5.3f} {(formula-mc)/mc:+4.2f}")
