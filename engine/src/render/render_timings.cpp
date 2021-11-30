#include "render_timings.h"

#include "render/vulkan/device.h"

#include <exo/base/logger.h>
#include <exo/time.h>
#include <exo/memory/string_repository.h>

RenderTimings RenderTimings::create(gfx::Device &d, StringRepository *r)
{
    RenderTimings result = {};
    d.create_query_pool(result.pool, TIMESTAMPS_PER_FRAME);
    result.str_repo = r;
    return result;
}

void RenderTimings::destroy(gfx::Device &device)
{
    device.destroy_query_pool(pool);
}

void RenderTimings::begin_label(gfx::Work &cmd, const char *label)
{
    if (began)
    {
        logger::error("labels can't be nested.\n");
        return;
    }

    labels.push_back(str_repo->intern(label));
    cmd.timestamp_query(pool, current_query);
    current_query += 1;

    cpu_ticks.push_back(Clock::now());

    began = true;
}

void RenderTimings::end_label(gfx::Work &cmd)
{
    if (!began)
    {
        logger::error("begin_label should be called before end_label.\n");
        return;
    }

    cmd.timestamp_query(pool, current_query);
    current_query += 1;

    cpu_ticks.push_back(Clock::now());

    began = false;
}

void RenderTimings::get_results(gfx::Device &device)
{
    if (began)
    {
        logger::error("label not ended.\n");
        return;
    }

    if (current_query == 0)
    {
        return;
    }

    device.get_query_results(pool, 0, current_query, gpu_ticks);

    for (u32 i = 0; i < labels.size(); i += 1) {
        gpu.push_back(
            1.e-6 * device.get_ns_per_timestamp() * static_cast<double>(gpu_ticks[2*i+1] - gpu_ticks[2*i])
            );

        cpu.push_back(
            elapsed_ms<double>(cpu_ticks[2*i], cpu_ticks[2*i+1])
            );
    }

}

void RenderTimings::reset(gfx::Device &device)
{
    device.reset_query_pool(pool, 0, TIMESTAMPS_PER_FRAME);

    cpu.clear();
    gpu.clear();
    cpu_ticks.clear();
    gpu_ticks.clear();
    labels.clear();
    current_query = 0;
    began = false;
}
