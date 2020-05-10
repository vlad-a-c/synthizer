import paths

import jinja2

import os.path
import pathlib

GEN_COMMENT = """
/*
 * This file is generated by the HRTF processor. Do not edit.
 */

"""

HEADER_TEMPLATE = GEN_COMMENT + """
#pragma once

#include <array>
#include <cstddef>

namespace synthizer::hrtf_data {

typedef std::array<std::array<float, {{ data.impulse_length }}>, {{ total_impulses }}> ImpulseArray;
extern const ImpulseArray IMPULSES;
const std::size_t IMPULSE_LENGTH = {{ data.impulse_length }};

struct ElevationDef {
    double angle;
    /*
     * Where the impulses start in the big array.
     */
    std::size_t azimuth_start;
    std::size_t azimuth_count;
};

extern const std::array<ElevationDef, {{ data.num_elevs }}> ELEVATIONS;

}
"""

SOURCE_TEMPLATE = GEN_COMMENT + """
#include <array>
#include <cstddef>
#include "synthizer/data/hrtf.hpp"

namespace synthizer::hrtf_data {

const ImpulseArray IMPULSES{ {
    {%- for i in data.azimuths %}
    {%- for j in i %}
    { {{ j | join(",") }} },
    {%- endfor %}
    {%- endfor %}
} };

const std::array<ElevationDef, {{ data.num_elevs }}> ELEVATIONS{ {
    {%- for e in elevation_defs %}
    { .angle = {{ e.angle }}, .azimuth_start = {{ e.azimuth_start }}, .azimuth_count = {{ e.azimuth_count }} },
    {%- endfor %}
} };

}
"""

def write_hrtf_data(data):
    elevation_defs = []
    total_impulses = 0
    for i, a in enumerate(data.azimuths):
        d = {
            "angle": data.elev_min + data.elev_increment *i,
            "azimuth_start": total_impulses,
            "azimuth_count": len(a),
        }
        total_impulses += len(a)
        elevation_defs.append(d)

    header = jinja2.Template(HEADER_TEMPLATE, undefined = jinja2.StrictUndefined)
    source = jinja2.Template(SOURCE_TEMPLATE, undefined = jinja2.StrictUndefined)
    header = header.render(data = data, elevation_defs = elevation_defs, total_impulses = total_impulses)
    source = source.render(data = data, elevation_defs = elevation_defs, total_impulses = total_impulses)

    header_path = os.path.join(paths.repo_path, "include", "synthizer", "data", "hrtf.hpp")
    source_path = os.path.join(paths.repo_path, "src", "data", "hrtf.cpp")

    for i in [header_path, source_path]:
        pathlib.Path(os.path.split(i)[0]).mkdir(parents = True, exist_ok = True)

    for p, c in [(source_path, source), (header_path, header)]:
        with open(p, "wb") as f:
            f.write(c.encode("utf-8"))
    