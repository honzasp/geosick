from .geosick import Ctx, Response, PointStream

# Determines the infection score and other metrics from two streams of interpolated user
# positions.
def meet(ctx: Ctx, sick_points: PointStream, query_points: PointStream) -> Response:
    pass

