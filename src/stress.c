#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <VapourSynth.h>
#include <VSHelper.h>

#define PLANES 3

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _WIN32
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline __attribute__((always_inline))
#endif

static FORCE_INLINE void write_frame(double *sum,
                                     VSFrameRef *dst,
                                     const int posx,
                                     const int posy,
                                     const int iterations,
                                     const VSAPI *vsapi)
{
    int plane;

    for (plane = 0; plane < PLANES; plane++) {
        sum[plane] = sum[plane] / (double)iterations;

        if (sum[plane] > 1)
            sum[plane] = 1.0;

        sum[plane] = sum[plane] * 255;

        uint8_t *dstp = vsapi->getWritePtr(dst, plane);
        int stride = vsapi->getStride(dst, plane);
        dstp[stride * posy + posx] = (uint8_t)ceil(sum[plane]);
    }

    memset(sum, 0, sizeof(double) * PLANES);
}

static void stress_iterations(VSFrameRef *dst,
                              const int width,
                              const int height,
                              const int iterations,
                              const int samples,
                              const int edge,
                              const VSFrameRef *src,
                              const VSAPI *vsapi)
{
    int x, y, plane;
    int nit, nsam;
    int dist, theta, xx, yy;
    double inc_x, inc_y;

    uint8_t max[PLANES] = {0};
    uint8_t min[PLANES] = {0};
    uint8_t rvalue[PLANES];

    double sum[PLANES] = {0};

    time_t t;
    srand((unsigned) time(&t));

    for (y = 0; y < height; y++) {
        for(x = 0; x < width; x++) {

            for (nit = 0; nit < iterations; nit++){

                for (nsam = 0; nsam < samples; nsam++) {

                    // Distance from the center of the pixel [0, edge]
                    dist = (rand() % edge) + 1;
                    // Angle from center of the pixel [0, 2pi]
                    theta = rand() % 361;

                    inc_x = dist * cos(theta * (M_PI / 180.0));
                    inc_y = dist * sin(theta * (M_PI / 180.0));

                    xx = abs((int)round((x + inc_x)));
                    yy = abs((int)round((y + inc_y)));

                    if (xx > width - 1 || yy > height - 1){
                        xx = rand() % width;
                        yy = rand() % height;
                    }

                    for (plane = 0; plane < PLANES; plane++) {
                        const uint8_t *srcp = vsapi->getReadPtr(src, plane);
                        const int stride = vsapi->getStride(src, plane);
                        max[plane] = VSMAX(srcp[stride * yy + xx], max[plane]);
                        min[plane] = VSMIN(srcp[stride * yy + xx], min[plane]);
                    }
                }// samples

                for (plane = 0; plane < PLANES; plane++) {
                    const uint8_t *srcp = vsapi->getReadPtr(src, plane);
                    const int stride = vsapi->getStride(src, plane);
                    max[plane] = VSMAX(srcp[stride * y + x], max[plane]);
                    min[plane] = VSMIN(srcp[stride * y + x], min[plane]);

                    rvalue[plane] = max[plane] - min[plane];

                    if (rvalue[plane] == 0)
                        sum[plane] = sum[plane] + 0.5;
                    else
                        sum[plane] = sum[plane] +
                                     ((srcp[stride * y + x] - min[plane]) /
                                     (double)rvalue[plane]);
                }

                memset(min, 0, PLANES);
                memset(max, 0, PLANES);
            } // iterations

            write_frame(sum, dst, x, y, iterations, vsapi);
        } // x
    } // y
}

static void rsr_iterations(VSFrameRef *dst,
                           const int width,
                           const int height,
                           const int iterations,
                           const int samples,
                           const int edge,
                           const VSFrameRef *src,
                           const VSAPI *vsapi)
{
    int x, y, plane;
    int nit, nsam;
    int dist, theta, xx, yy;
    double inc_x, inc_y;

    uint8_t max[PLANES] = {0};
    double sum[PLANES] = {0};

    time_t t;
    srand((unsigned) time(&t));

    for (y = 0; y < height; y++) {
        for(x = 0; x < width; x++) {
            for (nit = 0; nit < iterations; nit++) {
                for (nsam = 0; nsam < samples; nsam++) {

                    dist = (rand() % edge) + 1;
                    theta = rand() % 361;

                    inc_x = dist * cos(theta * (M_PI / 180.0));
                    inc_y = dist * sin(theta * (M_PI / 180.0));

                    xx = abs((int)round((x + inc_x)));
                    yy = abs((int)round((y + inc_y)));

                    if (xx > width - 1 || yy > height - 1) {
                        xx = rand() % width;
                        yy = rand() % height;
                    }

                    for (plane = 0; plane < PLANES; plane++) {
                        const uint8_t *srcp = vsapi->getReadPtr(src, plane);
                        const int stride = vsapi->getStride(src, plane);
                        max[plane] = VSMAX(srcp[stride * yy + xx], max[plane]);
                    }

                } // samples

                for (plane = 0; plane < PLANES; plane++) {
                    const uint8_t *srcp = vsapi->getReadPtr(src, plane);
                    const int stride = vsapi->getStride(src, plane);
                    max[plane] = VSMAX(srcp[stride * y + x], max[plane]);
                    if (max[plane] == 0)
                        sum[plane] = sum[plane] + 1;
                    else
                        sum[plane] = sum[plane] +
                                       (srcp[stride * y + x] /
                                       (double)max[plane]);
                }

                memset(max, 0, PLANES);

            } // iterations

            write_frame(sum, dst, x, y, iterations, vsapi);
        } // x
    } // y
}


typedef struct StressData {
    VSNodeRef *node;
    const VSVideoInfo *vi;

    int iterations;
    int samples;
    int ray; // 1 = image diagonal, 2 = half image diagonal
    int algorithm; // Choose between Stress/Rsr
} Stress;

static void VS_CC StressInit(VSMap *in,
                             VSMap *out,
                             void **instanceData,
                             VSNode *node,
                             VSCore *core,
                             const VSAPI *vsapi)
{
    Stress *d = (Stress *) * instanceData;
    vsapi->setVideoInfo(d->vi, 1, node);
}


static const VSFrameRef *VS_CC StressGetFrame(int n,
                                              int activationReason,
                                              void **instanceData,
                                              void **frameData,
                                              VSFrameContext *frameCtx,
                                              VSCore *core,
                                              const VSAPI *vsapi)
{
    Stress *d = (Stress *) * instanceData;

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->node, frameCtx);
    } else if (activationReason == arAllFramesReady) {
        const VSFrameRef *src = vsapi->getFrameFilter(n, d->node, frameCtx);

        const int height = vsapi->getFrameHeight(src, 0);
        const int width = vsapi->getFrameWidth(src, 0);
        const int edge = (int)(sqrt(pow(width, 2) + pow(height, 2)) / d->ray);

        VSFrameRef *dst = vsapi->copyFrame(src, core);

        if (d->algorithm == 0)
            stress_iterations(dst, width, height, d->iterations,
                              d->samples, edge, src, vsapi);
        else
            rsr_iterations(dst, width, height, d->iterations,
                           d->samples, edge, src, vsapi);

        vsapi->freeFrame(src);
        return dst;
    }

    return 0;
}

static void VS_CC StressFree(void *instanceData,
                             VSCore *core,
                             const VSAPI *vsapi)
{

    Stress *d = (Stress *)instanceData;

    vsapi->freeNode(d->node);
    free(d);
}

static void VS_CC StressCreate(const VSMap *in,
                               VSMap *out,
                               void *userData,
                               VSCore *core,
                               const VSAPI *vsapi)
{

    Stress d;
    Stress *data;

    int err;

    d.iterations = vsapi->propGetInt(in, "iterations", 0, &err);
    d.samples = vsapi->propGetInt(in, "samples", 0, &err);
    d.ray = vsapi->propGetInt(in, "ray", 0, &err);

    if (err)
        d.ray = 1;

    const char *algo_name = vsapi->propGetData(in, "algorithm", 0, &err);
    if (err) {
        d.algorithm = 0;
    } else {
        if (strcmp(algo_name, "Stress") == 0) {
            d.algorithm = 0;
        } else if (strcmp(algo_name, "Rsr") == 0) {
            d.algorithm = 1;
        } else {
            vsapi->setError(out,
                    "Stress: Not valid algorithm. "
                    "Insert 'Stress' or 'Rsr'");
            return;
        }
    }

    if (d.iterations < 1) {
        vsapi->setError(out, "Stress: iterations must be greater than 0.");
        return;
    }

    if (d.samples < 1) {
        vsapi->setError(out, "Stress: samples must be greater than 0.");
        return;
    }

    if (d.ray < 1 || d.ray > 2) {
        vsapi->setError(out, "Stress: ray can only be 1 or 2.");
        return;
    }

    d.node = vsapi->propGetNode(in, "clip", 0, 0);
    d.vi = vsapi->getVideoInfo(d.node);

    if (!isConstantFormat(d.vi) ||
        d.vi->format->sampleType != stInteger ||
        d.vi->format->bytesPerSample > 1 ||
        d.vi->format->id != pfRGB24)
    {
        vsapi->setError(out, "Stress: clip must have a RGB24 colorspace.");
        vsapi->freeNode(d.node);
        return;
    }

    data = malloc(sizeof(d));
    *data = d;

    vsapi->createFilter(in, out, "Stress", StressInit, StressGetFrame,
                        StressFree, fmParallel, 0, data, core);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc,
                                            VSRegisterFunction registerFunc,
                                            VSPlugin *plugin)
{
    configFunc("com.uni.stress", "st", "Stress/Rsr plugin for VapourSynth",
               VAPOURSYNTH_API_VERSION, 1, plugin);

    registerFunc("Stress",
                 "clip:clip;"
                 "iterations:int;"
                 "samples:int;"
                 "ray:int:opt;"
                 "algorithm:data:opt;",
                 StressCreate, 0, plugin);
}
