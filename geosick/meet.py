from .geosick import Ctx, Response, PointStream
import numpy as np

# Maximum distance at which one person may infect another
INFECT_RADIUS = 2.0 
# Maximum relative speed at which one person may infect another (this should be
# conservative, because the velocity estimates may be quite poor)
INFECT_MAX_SPEED = 1.0
# Probability that a person is infected by another person at distance at most
# INFECT_RADIUS in one minute
INFECT_RATE = 0.2
# Minimum infection rate that is considered for inclusion into Response.meet_ranges_ms
INFECT_MEET_THRESHOLD = 0.001

# Determines the infection score and other metrics from two streams of interpolated user
# positions.
def meet(ctx: Ctx, sick_points: PointStream, query_points: PointStream) -> Response:
    compl_score_log = 0.0
    min_distance = np.infinity
    meet_ranges = []
    first_meet = None
    last_meet = None

    for timestamp_ms, sick_p, query_p in zip(ctx.timestamps_ms, sick_points, query_points):
        if sick_p is not None and query_p is not None:
            # compute infection rate per minute and update the score
            rate = eval_infect_rate(ctx, sick_p, query_p)
            compl_score_log += np.log1p(-min(0.9, ctx.period_s/60 * rate))

            # take very conservative distance, try not to scare people too much!
            distance = np.linalg.norm(sick_p.pos - query_p.pos) \
                + sick_p.radius + query_p.radius
            min_distance = min(min_distance, distance)
        else:
            rate = 0

        if rate > INFECT_MEET_THRESHOLD:
            if first_meet is None:
                first_meet = timestamp_ms
            last_meet = timestamp_ms
        elif rate <= 0.5*INFECT_MEET_THRESHOLD:
            if first_meet is not None:
                meet_ranges.append((first_meet, last_meet))
            first_meet = last_meet = None

    if first_meet is not None:
        meet_ranges.append((first_meet, last_meet))

    score = 1 - np.exp(compl_score_log)
    return Response(score=score, distance=min_distance, meet_ranges=meet_ranges)

# Estimates the infection rate (infection probablity per minute) between sick_p and
# query_p
def eval_infect_rate(ctx, sick_p, query_p):
    distance = np.linalg.norm(sick_p.pos - query_p.pos)
    if distance >= sick_p.radius + query_p.radius: return 0

    speed = np.linalg.norm(sick_p.velocity - query_p.velocity)
    if speed >= INFECT_MAX_SPEED: return 0

    area_sick = np.pi * sick_p.radius**2
    area_query = np.pi * query_p.radius**2
    area_isect = circle_isect_area(sick_p.radius, query_p.radius, distance)
    area_infect = min(np.pi * INFECT_RADIUS**2, area_isect)

    return INFECT_RATE * (area_infect*area_isect) / (area_sick*area_query)

# Computes the area of the intersection of two circles of radius r1 and r2 at distance d
# https://mathworld.wolfram.com/Circle-CircleIntersection.html
def circle_isect_area(r1, r2, d):
    if r1 + r2 < d: return 0
    if d + r1 <= r2: return np.pi * r1**2
    if d + r2 <= r1: return np.pi * r2**2

    d1 = (d**2 - r2**2 + r1**2) / (2*d)
    d2 = d - d1
    a = np.sqrt((r1+r1+d)*(r1+r2-d)*(r1-r2-d)*(r2-r1-d)) / d
    theta1 = 2*np.acos(d1/r1)
    theta2 = 2*np.acos(d2/r2)
    return 0.5*(theta1*r1 + theta2*r2 - a*d)

