FROM python:3.8
WORKDIR /usr/src/app

COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt
RUN apt update -y && apt install -y libpq-dev pybind11-dev python3-dev

COPY geosick geosick
COPY libgeosick libgeosick
COPY meson.build meson.build

RUN meson build && ninja -C build && ninja -C build install

CMD [ "python", "-m", "geosick" ]
