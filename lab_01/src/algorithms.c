#include <stdio.h>

#include "../include/algorithms.h"
#include "../include/constants.h"

#define ABS(X) X > 0 ? X : -X

static int find_section(data_t *const data, const double argument, const int mode)
{
    // sign is positive if sequence is increasing, else - false (decreasing).
    float sign = (data->table[mode ^ 0][0] < data->table[mode ^ 0][1]) - 0.5;
    int index = 0;
    // explanation for increasing sequence:
    // until we didn't meet number, which is bigger - we keep searching,
    // else - we should be before it.
    while (index < data->size && (argument - data->table[mode ^ 0][index]) * sign > 0)
        index++;

    if (index == 0 || index == data->size)
    {
        index -= data->n;
        fprintf(OUTPUT, "Extrapolation!\n");
        return index > 0 ? index : 0;
    }
    
    // if behind found place there is no enough points, we'll take some from the beginning
    int end_diff = data->n / 2 - ~(1 & data->n) - (data->size - index - 1);
    //             half sector   not `even` sector     place left behind

    if (end_diff < 0)
        end_diff = 0;

    for (int i = 0; i < (data->n / 2 + (data->n & 1) + end_diff) && index > 0; i++)
        index--;
    return index; 
}


double interpolation(data_t *const data, const double argument,
                     const int mode, const int cache_usage)
{
    int section_start = find_section(data, argument, mode);

#ifdef DEBUG
    fprintf(OUTPUT, "Section start is in index: %d\nvalue: (%lf, %lf)\n", section_start,
            data->table[0][section_start], data->table[1][section_start]);
#endif

    // Find coefficients if there is no way to use cached data.
    if (!(cache_usage && mode == data->is_cached && section_start == data->cached_for))
    {

#ifdef DEBUG
        fprintf(OUTPUT, "Data is not cached:\n cached usage: %d\nmode %d\n is_cached %d\n"
                "data->cached_for %d\nsection_start %d\n", cache_usage, mode, data->is_cached,
                data->cached_for, section_start);
#endif

        for (int i = 0; i < data->n - 1; i++)
            data->divided_sums[i] = (data->table[1 ^ mode][i] - data->table[1 ^ mode][i + 1]) / 
                (data->table[0 ^ mode][i] - data->table[0 ^ mode][i + 1]);

        double *cur_elem = data->divided_sums + data->n - 1;
        double *sum_ptr = data->divided_sums;

        for (int i = 1; i < data->n; i++)
        {
            for (int j = 0; j < data->n - i - 1; j++)
            {
                *cur_elem = (*sum_ptr - *(sum_ptr + 1)) / 
                    (data->table[0 ^ mode][j] - data->table[0 ^ mode][j + 1 + i]);
                cur_elem++, sum_ptr++;
            }
            sum_ptr++;
        } 
    }        

#ifdef DEBUG
    fprintf(OUTPUT, "Divided sums:\n");
    for (int i = 0; i < (1 + data->n) * data->n / 2; i++)
        fprintf(OUTPUT, "%lf, ", data->divided_sums[i]);
    fprintf(OUTPUT, "\n\n");
#endif

    // Update cache info.
    data->is_cached = mode;
    data->cached_for = section_start;

    // Couning formula.
    double multi = 1;
    double summary = data->table[1 ^ mode][section_start];
    int index = 0;

    for (int i = 0; i < data->n; i++)
    {
        multi *= (argument - data->table[0 ^ mode][i]);
        summary += multi * data->divided_sums[index];
        index += data->n - i;
    }

    return summary;
}


double half_division(data_t *const data)
{
    double left[2] = { data->table[0][0], data->table[1][0] };
    double right[2] = { data->table[0][data->size - 1], data->table[0][data->size - 1] };
    
    if (left[1] * right[1] > 0)
    {
        fprintf(OUTPUT, "Function is not monotonous or doesn't cross axis!");
        return 0;
    }

    double center[2];

    center[0] = (left[0] + right[0]) / 2;
    while (ABS(center[0]) > data->acc)
    {
        center[1] = interpolation(data, center[0], NORMAL, TRUE);
        if (left[1] * center[1] > 0)
        {
            left[0] = center[0];
            left[1] = center[1];
        }
        else
        {
            right[0] = center[0];
            right[1] = center[1];
        }
        center[0] = (left[0] + right[0]) / 2;
    }
    return center[0];      
}
