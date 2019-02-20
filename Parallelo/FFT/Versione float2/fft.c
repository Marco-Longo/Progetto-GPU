#define CL_TARGET_OPENCL_VERSION 120
#include "ocl_boiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include "fft.h"

#define _2PI 6.283185307179586476925f
typedef cl_double2 complex_t;

void error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

cl_event matinit(cl_command_queue que, cl_kernel matinit_k, cl_mem d_mat,
                 cl_int nrows, cl_int ncols, int n_wait_events,
                 cl_event *wait_events)
{
    cl_int err;
    cl_int arg = 0;
    err = clSetKernelArg(matinit_k, arg++, sizeof(d_mat), &d_mat);
    ocl_check(err, "set matinit arg %d", arg - 1);
    err = clSetKernelArg(matinit_k, arg++, sizeof(nrows), &nrows);
    ocl_check(err, "set matinit arg %d", arg - 1);
    err = clSetKernelArg(matinit_k, arg++, sizeof(ncols), &ncols);
    ocl_check(err, "set matinit arg %d", arg - 1);

    const size_t lws[] = { 32, 8 };
    const size_t gws[] = {
        round_mul_up(ncols, lws[0]), //nel kernel get_global_id(0) è l'indice c
        round_mul_up(nrows, lws[1]), //nel kernel get_global_id(1) è l'indice r
    };

    cl_event evt_init;
    err = clEnqueueNDRangeKernel(que, matinit_k, 2, NULL, gws, lws,
                                 n_wait_events, wait_events, &evt_init);
    ocl_check(err, "enqueue matinit");

    return evt_init;
}
/*
cl_event product(cl_command_queue que, cl_kernel prod_k, cl_mem d_v1,
                 cl_mem d_v2, cl_mem d_v3, cl_int nrows, cl_int ncols, // dell'input
                 int n_wait_events, cl_event *wait_events)
{
    cl_int err;
    cl_int arg = 0;
    err = clSetKernelArg(prod_k, arg++, sizeof(d_v1), &d_v1);
    ocl_check(err, "set prod arg %d", arg - 1);
    err = clSetKernelArg(prod_k, arg++, sizeof(d_v2), &d_v2);
    ocl_check(err, "set prod arg %d", arg - 1);
    err = clSetKernelArg(prod_k, arg++, sizeof(d_v3), &d_v3);
    ocl_check(err, "set prod arg %d", arg - 1);
    err = clSetKernelArg(prod_k, arg++, sizeof(nrows), &nrows);
    ocl_check(err, "set prod arg %d", arg - 1);
    err = clSetKernelArg(prod_k, arg++, sizeof(ncols), &ncols);
    ocl_check(err, "set prod arg %d", arg - 1);

    const size_t lws[] = { 32, 8 };
    const size_t gws[] = {
        round_mul_up(ncols, lws[0]),
        round_mul_up(nrows, lws[1]),
    };

    cl_event evt_prod;
    err = clEnqueueNDRangeKernel(que, prod_k, 2, NULL, gws, lws,
                                 n_wait_events, wait_events, &evt_prod);
    ocl_check(err, "enqueue prod");

    return evt_prod;
}
*/
cl_event fft(cl_command_queue que, cl_kernel fft_k, cl_mem d_src_img, cl_mem d_dest_img,
             int u, int v, int nrows, int ncols, int n_wait_events, cl_event *wait_events)
{
    cl_int err;
    cl_int arg = 0;

    err = clSetKernelArg(fft_k, arg++, sizeof(d_src_img), &d_src_img);
    ocl_check(err, "set fft arg %d", arg - 1);
    err = clSetKernelArg(fft_k, arg++, sizeof(d_dest_img), &d_dest_img);
    ocl_check(err, "set fft arg %d", arg - 1);
    err = clSetKernelArg(fft_k, arg++, sizeof(u), &u);
    ocl_check(err, "set fft arg %d", arg - 1);
    err = clSetKernelArg(fft_k, arg++, sizeof(v), &v);
    ocl_check(err, "set fft arg %d", arg - 1);
    err = clSetKernelArg(fft_k, arg++, sizeof(nrows), &nrows);
    ocl_check(err, "set fft arg %d", arg - 1);
    err = clSetKernelArg(fft_k, arg++, sizeof(ncols), &ncols);
    ocl_check(err, "set fft arg %d", arg - 1);

    const size_t lws[] = {16, 16};
    const size_t gws[] = {
        round_mul_up(ncols, lws[0]),
        round_mul_up(nrows, lws[1]),
    };

    cl_event evt_fft;
    err = clEnqueueNDRangeKernel(que, fft_k, 2, NULL, gws, lws, n_wait_events,
                                 wait_events, &evt_fft);
    ocl_check(err, "enqueue fft");

    return evt_fft;
}

cl_event sum(cl_command_queue que, cl_kernel somma_k, cl_mem d_input, cl_mem d_output,
             cl_int numels, cl_int offset, size_t _lws, size_t _gws,
             int n_wait_events, cl_event *wait_events)
{
    cl_int err;
    cl_int arg = 0;

    err = clSetKernelArg(somma_k, arg++, sizeof(d_input), &d_input);
    ocl_check(err, "set somma arg %d", arg-1);
    err = clSetKernelArg(somma_k, arg++, sizeof(d_output), &d_output);
    ocl_check(err, "set somma arg %d", arg-1);
    err = clSetKernelArg(somma_k, arg++, _lws*sizeof(complex_t), NULL);
    ocl_check(err, "set vecsum arg %d", arg-1);
    err = clSetKernelArg(somma_k, arg++, sizeof(numels), &numels);
    ocl_check(err, "set somma arg %d", arg-1);
    err = clSetKernelArg(somma_k, arg++, sizeof(offset), &offset);
    ocl_check(err, "set somma arg %d", arg-1);

    const size_t lws[] = { _lws };
    const size_t gws[] = { _gws };

    cl_event evt_sum;

    err = clEnqueueNDRangeKernel(que, somma_k, 1, NULL, gws, lws, n_wait_events,
                                 wait_events, &evt_sum);
    ocl_check(err, "enqueue sum");

    return evt_sum;
}

void verify(complex** reale, int nrows, int ncols)
{
    int** input = init(nrows, ncols);
    complex** atteso = fft_c(input, nrows, ncols);

    for (int x = 0; x < nrows; ++x)
        for (int y = 0; y < ncols; ++y)
        {
            if((atteso[x][y].real-reale[x][y].real)>1.0e-7 ||
               (atteso[x][y].imag-reale[x][y].imag)>1.0e-7)
                  fprintf(stderr, "mismatch@ %d %d: %.9g+%.9gi != %.9g+%.9gi\n", x, y,
                          reale[x][y].real, reale[x][y].imag, atteso[x][y].real,
                          atteso[x][y].imag);
        }
}

int main(int argc, char *argv[])
{
    if (argc != 5)
        error("sintassi: product nrows ncols lws nwg");

    const int nrows = atoi(argv[1]);
    const int ncols = atoi(argv[2]);
    const size_t lws0 = atoi(argv[3]);
    const size_t nwg = atoi(argv[4]);

    const int numels = nrows*ncols;
    const size_t memsize = numels*sizeof(int);
    const size_t memsize_complex = numels*sizeof(complex_t);

    if (nrows <= 0 || ncols <= 0)
        error("nrows, ncols devono essere positivi");
    if (numels & (numels-1))
        error("numels deve essere una potenza di due");

    cl_platform_id p = select_platform();
    cl_device_id d = select_device(p);
    cl_context ctx = create_context(p, d);
    cl_command_queue que = create_queue(ctx, d);
    cl_program prog = create_program("fft.ocl", ctx, d);

    cl_int err;

    cl_kernel matinit_k = clCreateKernel(prog, "matinit", &err);
    ocl_check(err, "create kernel matinit");

    cl_kernel fft_k = clCreateKernel(prog, "fftx4", &err);
    ocl_check(err, "create kernel fft");

    cl_kernel somma_k = clCreateKernel(prog, "somma_lmem", &err);
    ocl_check(err, "create kernel somma");

    const size_t gws0 = nwg*lws0;
    size_t memsize_min = gws0*sizeof(complex_t);

    /* Allocazione buffer */
    cl_mem d_v1 = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
                                 memsize, NULL, &err);
    ocl_check(err, "create buffer v1");
    cl_mem d_v2 = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                 memsize_complex > memsize_min ? 4*memsize_complex : 4*memsize_min, NULL, &err);
    ocl_check(err, "create buffer v2");
    cl_mem d_vsum = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                   4*memsize_min, NULL, &err);
    ocl_check(err, "create buffer sum");

    complex **res_fft = init_complex(nrows, ncols);

    //Inizializzazione
    cl_event evt_init = matinit(que, matinit_k, d_v1, nrows, ncols, 0, NULL);

    //Trasformata
    cl_event evt_start, evt_end;

    for(int u=0; u<nrows/4; ++u)
        for(int v=0; v<ncols; ++v)
        {
            cl_event evt_fft = fft(que, fft_k, d_v1, d_v2, u, v,
                                   nrows, ncols, 1, &evt_init);
            if(u==0 && v==0)
                memcpy(&evt_start, &evt_fft, sizeof(cl_event));

            //RIDUZIONE
            size_t nsums = (nwg == 1 ? 1 : 2);
            const size_t lws_final = 32;
            cl_event evt_sum[8];

            //Prima chiamata (offset 0)
            evt_sum[0] = sum(que, somma_k, d_v2, d_vsum, numels, 0,
                             lws0, gws0, 1, &evt_fft);
            if(nwg > 1)
                evt_sum[1] = sum(que, somma_k, d_vsum, d_vsum, nwg, 0,
                                 lws_final, lws_final, 1, evt_sum);

            // //Seconda chiamata (offset numels / nwg)
            // evt_sum[2] = sum(que, somma_k, d_v2, d_vsum, numels, numels,
            //                  lws0, gws0, 1, (evt_sum + nsums -1));
            // if(nwg > 1)
            //     evt_sum[3] = sum(que, somma_k, d_vsum, d_vsum, nwg, nwg,
            //                      lws_final, lws_final, 1, evt_sum+2);

            for(int h=1; h<4; ++h)
            {
                evt_sum[2*h] = sum(que, somma_k, d_v2, d_vsum, numels, h,
                                   lws0, gws0, 0, NULL);
                if(nwg > 1)
                    evt_sum[2*h+1] = sum(que, somma_k, d_vsum, d_vsum, nwg, h,
                                         lws_final, lws_final, 1, evt_sum+2*h);
            }


            if(u==(nrows/4)-1 && v==ncols-1)
                memcpy(&evt_end, &evt_sum[nsums+5], sizeof(cl_event));

            err = clFinish(que);
            ocl_check(err, "clFinish");

            //Estrazione risultato
            complex_t *res = malloc(4*sizeof(complex_t));
            err = clEnqueueReadBuffer(que, d_vsum, CL_TRUE, 0,
                                      4*sizeof(complex_t), res, 0, NULL, NULL);
            ocl_check(err, "read buffer vsum");
            res_fft[u][v].real = res[0].s[0];
            res_fft[u][v].imag = res[0].s[1];
            res_fft[u+(nrows/4)][v].real = res[1].s[0];
            res_fft[u+(nrows/4)][v].imag = res[1].s[1];
            res_fft[u+(nrows/2)][v].real = res[2].s[0];
            res_fft[u+(nrows/2)][v].imag = res[2].s[1];
            res_fft[u+(nrows*3/4)][v].real = res[3].s[0];
            res_fft[u+(nrows*3/4)][v].imag = res[3].s[1];
        }

    printf("totale: %gms\t%gGB/s\n", total_runtime_ms(evt_start, evt_end),
           ((2.0*memsize_complex*numels)/total_runtime_ns(evt_start, evt_end)));

    verify(res_fft, nrows, ncols);

    clReleaseMemObject(d_v1);
    clReleaseMemObject(d_v2);
    clReleaseMemObject(d_vsum);
    free(res_fft);

    clReleaseKernel(matinit_k);
    clReleaseKernel(fft_k);
    clReleaseKernel(somma_k);
    clReleaseProgram(prog);
    clReleaseCommandQueue(que);
    clReleaseContext(ctx);
    return 0;
}
