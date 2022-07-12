
#include <immintrin.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>
#include <iostream>

#include "nnue.h"
#include "structs.h"
#include "thread.h"
#include "incbin.h"

extern "C" {

    INCBIN(Network, "network.nnue");
}

#define WEIGHT_SCALE_BITS 6

/* Definition of the network architcechure */
#define NET_VERSION 0x00000005
#define NET_HEADER_SIZE 4
#define NUM_INPUT_FEATURES 64*64*10
#define MAX_ACTIVE_FEATURES 30
#define NUM_LAYERS 4
#define HALFKX_LAYER_SIZE 128

#define nnue2score 600.0f
#define weight_scale_hidden 64.0f
#define weight_scale_out 16.0f
#define quantized_one 127.0f

float inputLayerWeightScale = quantized_one;
float inputLayerBiasScale = quantized_one;

float hiddenLayerWeightScale = weight_scale_hidden;
float hiddenLayerBiasScale = weight_scale_hidden * quantized_one;

float outputLayerWeightScale = nnue2score * weight_scale_out / quantized_one;
float outputLayerBiasScale = weight_scale_out * nnue2score;

constexpr int model_layer_sizes[NUM_LAYERS] = {HALFKX_LAYER_SIZE*2, 32, 32, 1};

uint32_t read_uint32_le(uint8_t *buffer)
{
    uint32_t val;
    uint8_t  *p = (uint8_t*)&val;

#ifndef TARGET_BIG_ENDIAN
    p[0] = buffer[0];
    p[1] = buffer[1];
    p[2] = buffer[2];
    p[3] = buffer[3];
#else
    p[3] = buffer[0];
    p[2] = buffer[1];
    p[1] = buffer[2];
    p[0] = buffer[3];
#endif
    return val;
}

template<typename T1, typename T2, U64 size1, U64 size2>
struct alignas(64) LinearLayer {

    T1 weights[size1];
    T2 biases[size2];
};

template<int size1, int size2>
struct alignas(64) LayerData {

    int32_t internal[size1];
    uint8_t external[size2];
};

#define weight_scale_hidden 64.0f
#define weight_scale_out 16.0f
#define quantized_one 127.0f

template <int s>
struct LayerSize {

    static constexpr int size = s;
};


LinearLayer<int16_t, int16_t, HALFKX_LAYER_SIZE * NUM_INPUT_FEATURES, HALFKX_LAYER_SIZE> layer0;
LinearLayer<int8_t, int32_t, 32 * HALFKX_LAYER_SIZE * 2, 32> layer1;
LinearLayer<int8_t, int32_t, 32 * 32, 32> layer2;
LinearLayer<int8_t, int32_t, 1 * 32, 1> layer3;

LayerSize<HALFKX_LAYER_SIZE * 2> layer0_size;
LayerSize<32> layer1_size;
LayerSize<32> layer2_size;
LayerSize<1> layer3_size;




void refresh_accumulator(
    Thread* th,
    Side side
) {

    constexpr int register_width = 256 / 16;
    static_assert(HALFKX_LAYER_SIZE % register_width == 0, "We're processing 16 elements at a time");
    constexpr int num_chunks = HALFKX_LAYER_SIZE / register_width;

    __m256i regs[num_chunks];


    // Load bias to registers and operate on registers only.
    for (int i = 0; i < num_chunks; ++i) {

        regs[i] = _mm256_load_si256((const __m256i*)(&layer0.biases[i * register_width]));
    }


    auto kingSq = side == WHITE ?
        GET_POSITION(th->whitePieceBB[KING]) : GET_POSITION(th->blackPieceBB[KING]) ^ 63;


    // TODO check logic
    for (int piece = PAWNS - 1 ; piece < KING - 1; piece++) {

        auto pieceBB = th->whitePieceBB[piece + 1];
        auto piece_color = WHITE;


        while (pieceBB) {

            auto pieceSq = GET_POSITION(pieceBB);
            POP_POSITION(pieceBB);

            pieceSq = side == WHITE ? pieceSq : pieceSq ^ 63;

            auto index = (piece << 1) + (side == BLACK ? 1 - piece_color : piece_color);

            auto feature = (640 * kingSq) + (64 * index) + (pieceSq);

            for (int i = 0; i < num_chunks; ++i) {

                regs[i] = _mm256_add_epi16(
                    regs[i], _mm256_load_si256((const __m256i*)(&layer0.weights[NN_SIZE * feature + (i * register_width)])));
            }
        }


        pieceBB = th->blackPieceBB[piece + 1];
        piece_color = BLACK;

        while (pieceBB) {

            auto pieceSq = GET_POSITION(pieceBB);
            POP_POSITION(pieceBB);

            pieceSq = side == WHITE ? pieceSq : pieceSq ^ 63;

            auto index = (piece << 1) + (side == BLACK ? 1 - piece_color : piece_color);

            auto feature = (640 * kingSq) + (64 * index) + (pieceSq);

            for (int i = 0; i < num_chunks; ++i) {

                regs[i] = _mm256_add_epi16(
                    regs[i], _mm256_load_si256((const __m256i*)(&layer0.weights[NN_SIZE * feature + (i * register_width)])));
            }
        }
    }


    for (int i = 0; i < num_chunks; ++i) {

        _mm256_store_si256((__m256i*)&th->accumulator[side][i * register_width], regs[i]);
    }
}




void update_accumulator(Thread* th, const std::vector< int >& removed_features, const std::vector< int >& added_features, Side side)
{

    constexpr int register_width = 256 / 16;
    static_assert(HALFKX_LAYER_SIZE % register_width == 0, "We're processing 16 elements at a time");
    constexpr int num_chunks = HALFKX_LAYER_SIZE / register_width;

    __m256i regs[num_chunks];

    for (int i = 0; i < num_chunks; ++i) {

        regs[i] = _mm256_load_si256(
                      reinterpret_cast<__m256i*>(&th->accumulator[side][i * register_width]));
    }

    for (int r : removed_features) {
        for (int i = 0; i < num_chunks; ++i) {

            regs[i] = _mm256_sub_epi16(
                regs[i], _mm256_load_si256((const __m256i*)&layer0.weights[NN_SIZE * r + (i * register_width)]));
        }
    }

    for (int a : added_features) {
        for (int i = 0; i < num_chunks; ++i) {

            regs[i] = _mm256_add_epi16(
                regs[i], _mm256_load_si256((const __m256i*)&layer0.weights[NN_SIZE * a + (i * register_width)]));
        }
    }

    for (int i = 0; i < num_chunks; ++i) {

        _mm256_store_si256((__m256i*)&th->accumulator[side][i * register_width], regs[i]);
    }
}






static void m256_add_dpbusd_epi32(__m256i& acc, __m256i a, __m256i b)
{
#if defined (USE_VNNI)
    acc = _mm256_dpbusd_epi32(acc, a, b);
#else
    __m256i product0 = _mm256_maddubs_epi16(a, b);
    product0 = _mm256_madd_epi16(product0, _mm256_set1_epi16(1));
    acc = _mm256_add_epi32(acc, product0);
#endif
}

static __m128i m256_haddx4(__m256i a, __m256i b, __m256i c, __m256i d)
{
    a = _mm256_hadd_epi32(a, b);
    c = _mm256_hadd_epi32(c, d);
    a = _mm256_hadd_epi32(a, c);
    const __m128i sum128lo = _mm256_castsi256_si128(a);
    const __m128i sum128hi = _mm256_extracti128_si256(a, 1);
    return _mm_add_epi32(sum128lo, sum128hi);
}

__m128i m256_haddx4(__m256i sum0, __m256i sum1, __m256i sum2, __m256i sum3, __m128i bias) {
    sum0 = _mm256_hadd_epi32(sum0, sum1);
    sum2 = _mm256_hadd_epi32(sum2, sum3);

    sum0 = _mm256_hadd_epi32(sum0, sum2);

    __m128i sum128lo = _mm256_castsi256_si128(sum0);
    __m128i sum128hi = _mm256_extracti128_si256(sum0, 1);

    return _mm_add_epi32(_mm_add_epi32(sum128lo, sum128hi), bias);
};



void hiddenLayer(
    int32_t*           output,
    uint8_t*           input,
    const int8_t*      weights,
    const int32_t*     biases,
    int      num_inputs,
    int      num_outputs) {

    constexpr int register_width = 256 / 8;

    const int num_out_chunks = num_outputs / 4;

    const int num_in_chunks = num_inputs / register_width;

    for (int i = 0; i < num_out_chunks; ++i) {

        const int offset0 = (i * 4 + 0) * num_inputs;
        const int offset1 = (i * 4 + 1) * num_inputs;
        const int offset2 = (i * 4 + 2) * num_inputs;
        const int offset3 = (i * 4 + 3) * num_inputs;

        __m256i sum0 = _mm256_setzero_si256();
        __m256i sum1 = _mm256_setzero_si256();
        __m256i sum2 = _mm256_setzero_si256();
        __m256i sum3 = _mm256_setzero_si256();

        for (int j = 0; j < num_in_chunks; ++j) {

            const __m256i in =
                _mm256_load_si256(reinterpret_cast<const __m256i*>(&input[j * register_width]));

            m256_add_dpbusd_epi32(sum0, in, _mm256_load_si256(
                    reinterpret_cast<const __m256i*>(&weights[offset0 + j * register_width])));
            m256_add_dpbusd_epi32(sum1, in, _mm256_load_si256(
                    reinterpret_cast<const __m256i*>(&weights[offset1 + j * register_width])));
            m256_add_dpbusd_epi32(sum2, in, _mm256_load_si256(
                    reinterpret_cast<const __m256i*>(&weights[offset2 + j * register_width])));
            m256_add_dpbusd_epi32(sum3, in, _mm256_load_si256(
                    reinterpret_cast<const __m256i*>(&weights[offset3 + j * register_width])));
        }

        __m128i bias = _mm_load_si128((const __m128i*)&biases[i*4]);
        __m128i outval =  m256_haddx4(sum0, sum1, sum2, sum3, bias);

        outval = _mm_srai_epi32(outval, WEIGHT_SCALE_BITS);


        _mm_store_si128((__m128i*)&output[i*4], outval);
    }
}


void outerLayer(
    int32_t*           output,
    uint8_t*           input,
    const int8_t*      weights,
    const int32_t*     bias,
    int      num_inputs) {

    constexpr uint32_t registerWidth = 256 / 8;

    __m256i sum = _mm256_setzero_si256();

    for (uint32_t j = 0; j < num_inputs; j += registerWidth)
    {
        const __m256i in = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + j));

        m256_add_dpbusd_epi32(sum, in, _mm256_load_si256(reinterpret_cast<const __m256i*>(weights + j)));
    }

    const __m128i sum128lo = _mm256_castsi256_si128(sum);
    const __m128i sum128hi = _mm256_extracti128_si256(sum, 1);

    const __m128i r4 = _mm_add_epi32(sum128lo, sum128hi);

    const __m128i r2 = _mm_add_epi32(r4, _mm_srli_si128(r4, 8));
    const __m128i r1 = _mm_add_epi32(r2, _mm_srli_si128(r2, 4));

    output[0] = _mm_cvtsi128_si32(r1) + bias[0];
}



void crelu_16(
    int      size,   // no need to have any layer structure, we just need the number of elements
    uint8_t*  output, // the already allocated storage for the result
    const int16_t* input   // the input, which is the output of the previous linear layer
) {

    constexpr int in_register_width = 256 / 16;
    constexpr int out_register_width = 256 / 8;
    //assert(size % out_register_width == 0, "We're processing 32 elements at a time");
    const int num_out_chunks = size / out_register_width;

    const __m256i zero    = _mm256_setzero_si256();
    const int     control = 0b11011000; // 3, 1, 2, 0; lane 0 is the rightmost one

    for (int i = 0; i < num_out_chunks; ++i) {

        const __m256i in0 = _mm256_load_si256(
                                reinterpret_cast<const __m256i*>(&input[(i * 2 + 0) * in_register_width]));

        const __m256i in1 = _mm256_load_si256(
                                reinterpret_cast<const __m256i*>(&input[(i * 2 + 1) * in_register_width]));

        const __m256i result =
            // packs changes the order, so we need to fix that with a permute
            _mm256_permute4x64_epi64(

                // clamp from below
                _mm256_max_epi8(
                    // packs saturates to 127, so we only need to clamp from below
                    _mm256_packs_epi16(in0, in1),

                    zero
                ),

                control
            );

        _mm256_store_si256(reinterpret_cast<__m256i*>(&output[i * out_register_width]), result);
    }
}



void crelu_32(

    int      size,   // no need to have any layer structure, we just need the number of elements
    uint8_t*  output, // the already allocated storage for the result
    const int32_t* input) {   // the input, which is the output of the previous linear layer


    constexpr int in_register_width = 256 / 32;
    constexpr int out_register_width = 256 / 8;
    //__assert (size % out_register_width == 0, "We're processing 32 elements at a time");
    const int num_out_chunks = size / out_register_width;

    const __m256i zero    = _mm256_setzero_si256();
    const __m256i control = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);


    for (int i = 0; i < num_out_chunks; ++i) {

        const __m256i in0 =
            _mm256_packs_epi32(
                _mm256_load_si256(
                    reinterpret_cast<const __m256i*>(&input[(i * 4 + 0) * in_register_width])),

                _mm256_load_si256(
                    reinterpret_cast<const __m256i*>(&input[(i * 4 + 1) * in_register_width]))
            );

        const __m256i in1 =
            _mm256_packs_epi32(
                _mm256_load_si256(
                    reinterpret_cast<const __m256i*>(&input[(i * 4 + 2) * in_register_width])),
                _mm256_load_si256(
                    reinterpret_cast<const __m256i*>(&input[(i * 4 + 3) * in_register_width]))
            );

        const __m256i result =
            _mm256_permutevar8x32_epi32(
                _mm256_max_epi8(
                    _mm256_packs_epi16(in0, in1),
                    zero
                ),
                control
            );

        _mm256_store_si256(reinterpret_cast<__m256i*>(&output[i * out_register_width]), result);
    }
}



static bool parse_header(uint8_t **data)
{
    uint8_t  *iter = *data;
    uint32_t version;

    version = read_uint32_le(iter);
    iter += 4;

    *data = iter;

    //return version == NET_VERSION;

    return true;
}



static bool parse_network(uint8_t **data)
{
    float *iter = (float*)*data;
    int   k;
    int   l;

    // INPUT / FEATURE LAYER

    for (k = 0; k < HALFKX_LAYER_SIZE; k++, iter++)
        layer0.biases[k] = *iter * inputLayerBiasScale;

    for (k=0; k<HALFKX_LAYER_SIZE * NUM_INPUT_FEATURES; k++, iter++)
        layer0.weights[k] = *iter * inputLayerWeightScale;


    // HIDDEN LAYER 1

    for (k=0; k<32; k++,iter++)
        layer1.biases[k] = *iter * hiddenLayerBiasScale;

    for (k=0; k<32; k++) {

        for (l=0; l<HALFKX_LAYER_SIZE * 2; l++,iter++)

            //column-major
            layer1.weights[k*(HALFKX_LAYER_SIZE * 2)+l]
                = *iter * hiddenLayerWeightScale;
    }

    //HIDDEN LAYER 2

    for (k=0; k<32; k++,iter++)
        layer2.biases[k] = *iter * hiddenLayerBiasScale;

    for (k=0; k<32; k++) {
        for (l=0; l<32; l++,iter++)
            layer2.weights[k*32+l] = *iter * hiddenLayerWeightScale;
    }


    // OUTPUT LAYER

    for (k=0; k<1; k++,iter++)
        layer3.biases[k] = *iter * outputLayerBiasScale;


    for (k=0; k<1; k++) {
        for (l=0; l<32; l++,iter++)
            layer3.weights[k*32+l] = *iter * outputLayerWeightScale;
    }


    *data = (uint8_t*)iter;


    return true;
}


static uint32_t calculate_net_size(void)
{
    uint32_t size = 0;
    int      k;

    size += NET_HEADER_SIZE;

    size += HALFKX_LAYER_SIZE*sizeof(float);
    size += HALFKX_LAYER_SIZE*NUM_INPUT_FEATURES*sizeof(float);

    for (k=1; k<NUM_LAYERS; k++) {
        size += model_layer_sizes[k]*sizeof(float);
        size += model_layer_sizes[k]*model_layer_sizes[k-1]*sizeof(float);
    }

    return size;
}



#include <sys/stat.h>

uint32_t get_file_size(char *file)
{
    assert(file != NULL);

#ifdef WINDOWS
    HANDLE fh;
    DWORD  size;

    fh = CreateFile(file, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    size = GetFileSize(fh, NULL);
    CloseHandle(fh);

    return (uint32_t)size;
#else
    struct stat sb;

    if (stat(file, &sb) != 0) {
        return 0xFFFFFFFF;
    }
    return (uint32_t)sb.st_size;
#endif
}


bool loadNetwork() {

    uint32_t size;
    uint8_t  *data = NULL;
    uint8_t  *iter;

    data = (uint8_t*)gNetworkData;
    size = (uint32_t)gNetworkSize;

    if (size != calculate_net_size())
        return false;

    /* Parse network header */
    iter = data;
    if (!parse_header(&iter))
        return false;

    /* Parse network */
    if (!parse_network(&iter))
        return false;

    return true;
}


bool nnue_load_net(std::string path)
{
    uint32_t size;
    size_t   count;
    uint8_t  *data = NULL;
    uint8_t  *iter;
    FILE     *fh = NULL;
    bool     ret = true;

    /* If an external net is specified then read the complete file */
    if (!path.empty()) {

        size = get_file_size(const_cast<char*>(path.c_str()));
        if (size != calculate_net_size()) {
            ret = false;
            goto exit;
        }

        fh = fopen(const_cast<char*>(path.c_str()), "rb");
        if (fh == NULL) {
            ret = false;
            goto exit;
        }


        data = (uint8_t*)malloc(size);


        count = fread(data, 1, size, fh);
        if (count != size) {

            ret = false;
            goto exit;
        }
    } else {

        data = (uint8_t*)gNetworkData;
        size = (uint32_t)gNetworkSize;

         if (size != calculate_net_size()) {
             ret = false;
             goto exit;
         }

        //  data = (uint8_t*)nnue_net_data;
        //  size = (uint32_t)nnue_net_size;
        //  if (size != calculate_net_size()) {
        //      ret = false;
        //      goto exit;
        //  }
    }

    /* Parse network header */
    iter = data;
    if (!parse_header(&iter)) {
        ret = false;
        goto exit;
    }

    /* Parse network */
    if (!parse_network(&iter)) {
        ret = false;
        goto exit;
    }

exit:
    if (!path.empty()) {
        free(data);
        if (fh != NULL) {
            fclose(fh);
        }
    }
    return ret;
}


int predict (Side stm, Thread *th) {


    LayerData<layer1_size.size, layer1_size.size> layer1_data;
    LayerData<layer2_size.size, layer2_size.size> layer2_data;
    LayerData<layer3_size.size, layer3_size.size> layer3_data;


    int16_t input[layer0_size.size];
    alignas(64) uint8_t output0[layer0_size.size];

    // TODO Use perspective after training with data that uses both perspectives
    //auto perspective = stm == WHITE ? 0 : 1;

    int offset = layer0_size.size / 2;
    for (int i = 0; i < layer0_size.size / 2; i++) {

        input[i] = th->accumulator[WHITE][i];
        input[offset + i] = th->accumulator[BLACK][i];
    }


    crelu_16 (layer0_size.size, output0, input);


    // HIDDEN LAYER 1

    hiddenLayer(layer1_data.internal, output0, layer1.weights,
                       layer1.biases, layer0_size.size, layer1_size.size);

    crelu_32 (layer1_size.size, layer1_data.external, layer1_data.internal);


    // HIDDEN LAYER 2

    hiddenLayer(layer2_data.internal, layer1_data.external, layer2.weights,
                       layer2.biases, layer1_size.size, layer2_size.size);

    crelu_32 (layer2_size.size, layer2_data.external, layer2_data.internal);


    // OUTPUT LAYER

    outerLayer(layer3_data.internal, layer2_data.external, layer3.weights,
              layer3.biases, layer2_size.size);


    int score = layer3_data.internal[0] / weight_scale_out;


    //return score;

    return stm == WHITE ? score : - score;
}









