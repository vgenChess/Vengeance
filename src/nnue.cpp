
#include <immintrin.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>
#include <iostream>
#include <fstream>

#include "nnue.h"
#include "structs.h"

#define NNUE2SCORE              600.0f
#define WEIGHT_SCALE_HIDDEN     64.0f
#define WEIGHT_SCALE_OUT        16.0f
#define QUANTIZED_ONE           127.0f

#define WEIGHT_SCALE_BITS       6
#define TOTAL_FEATURES          40960
#define NUM_LAYERS              4
#define FEATURE_LAYER_SIZE      128

constexpr float INPUT_LAYER_SCALE_WEIGHT    = QUANTIZED_ONE;
constexpr float INPUT_LAYER_SCALE_BIAS      = QUANTIZED_ONE;
constexpr float HIDDEN_LAYER_SCALE_WEIGHT   = WEIGHT_SCALE_HIDDEN;
constexpr float HIDDEN_LAYER_SCALE_BIAS     = WEIGHT_SCALE_HIDDEN * QUANTIZED_ONE;
constexpr float OUTPUT_LAYER_SCALE_WEIGHT   = NNUE2SCORE * WEIGHT_SCALE_OUT / QUANTIZED_ONE;
constexpr float OUTPUT_LAYER_SCALE_BIAS     = WEIGHT_SCALE_OUT * NNUE2SCORE;

constexpr int layerSize[NUM_LAYERS] = {FEATURE_LAYER_SIZE * 2, 32, 32, 1};

template<typename T1, typename T2, U64 size1, U64 size2>
struct alignas(64) LinearLayer {

    T1 weights[size1] = {};
    T2 biases[size2] = {};
};

template<int size1, int size2>
struct alignas(64) LayerData {

    int32_t internal[size1] = {};
    uint8_t external[size2] = {};
};


// TODO refactor=====================================================================
#define NET_VERSION             0x00000005
#define NET_HEADER_SIZE         4
//===================================================================================


LinearLayer<int16_t, int16_t, FEATURE_LAYER_SIZE * TOTAL_FEATURES, FEATURE_LAYER_SIZE> firstLayer;
LinearLayer<int8_t, int32_t, 32 * FEATURE_LAYER_SIZE * 2, 32> secondLayer;
LinearLayer<int8_t, int32_t, 32 * 32, 32> thirdLayer;
LinearLayer<int8_t, int32_t, 1 * 32, 1> fourthLayer;

void refresh_accumulator(
    GameInfo* gi,
    Side side) {

    constexpr int register_width = 256 / 16;
    static_assert(FEATURE_LAYER_SIZE % register_width == 0, "We're processing 16 elements at a time");
    constexpr int num_chunks = FEATURE_LAYER_SIZE / register_width;

    __m256i regs[num_chunks];

    // Load bias to registers and operate on registers only.
    for (int i = 0; i < num_chunks; ++i)
        regs[i] = _mm256_load_si256((const __m256i*)(&firstLayer.biases[i * register_width]));


    auto kingSq = side == WHITE ?
        GET_POSITION( gi->whitePieceBB[KING]) : GET_POSITION( gi->blackPieceBB[KING]) ^ 63;


    // TODO check logic
    for (int piece = PAWNS - 1 ; piece < KING - 1; piece++) {

        auto pieceBB = gi->whitePieceBB[piece + 1];
        auto piece_color = WHITE;


        while (pieceBB) {

            auto pieceSq = GET_POSITION(pieceBB);
            POP_POSITION(pieceBB);

            pieceSq = side == WHITE ? pieceSq : pieceSq ^ 63;

            auto index = (piece << 1) + (side == BLACK ? 1 - piece_color : piece_color);

            auto feature = (640 * kingSq) + (64 * index) + (pieceSq);

            for (int i = 0; i < num_chunks; ++i) {

                regs[i] = _mm256_add_epi16(
                    regs[i], _mm256_load_si256((const __m256i*)(&firstLayer.weights[NN_SIZE * feature + (i * register_width)])));
            }
        }

        pieceBB = gi->blackPieceBB[piece + 1];
        piece_color = BLACK;

        while (pieceBB) {

            auto pieceSq = GET_POSITION(pieceBB);
            POP_POSITION(pieceBB);

            pieceSq = side == WHITE ? pieceSq : pieceSq ^ 63;

            auto index = (piece << 1) + (side == BLACK ? 1 - piece_color : piece_color);

            auto feature = (640 * kingSq) + (64 * index) + (pieceSq);

            for (int i = 0; i < num_chunks; ++i) {

                regs[i] = _mm256_add_epi16(
                    regs[i], _mm256_load_si256((const __m256i*)(&firstLayer.weights[NN_SIZE * feature + (i * register_width)])));
            }
        }
    }

    for (int i = 0; i < num_chunks; ++i)
        _mm256_store_si256((__m256i*)&gi->accumulator[side][i * register_width], regs[i]);
}

void update_accumulator (
    GameInfo* gi,
    const std::vector< int >& removed_features,
    const std::vector< int >& added_features,
    Side side ) {

    constexpr int register_width = 256 / 16;
    static_assert(FEATURE_LAYER_SIZE % register_width == 0, "We're processing 16 elements at a time");
    constexpr int num_chunks = FEATURE_LAYER_SIZE / register_width;

    __m256i regs[num_chunks];

    for (int i = 0; i < num_chunks; ++i) {

        regs[i] = _mm256_load_si256(
                      reinterpret_cast<__m256i*>(&gi->accumulator[side][i * register_width]));
    }

    for (int r : removed_features) {
        for (int i = 0; i < num_chunks; ++i) {

            regs[i] = _mm256_sub_epi16(
                regs[i], _mm256_load_si256((const __m256i*)&firstLayer.weights[NN_SIZE * r + (i * register_width)]));
        }
    }

    for (int a : added_features) {
        for (int i = 0; i < num_chunks; ++i) {

            regs[i] = _mm256_add_epi16(
                regs[i], _mm256_load_si256((const __m256i*)&firstLayer.weights[NN_SIZE * a + (i * register_width)]));
        }
    }

    for (int i = 0; i < num_chunks; ++i)
        _mm256_store_si256((__m256i*)&gi->accumulator[side][i * register_width], regs[i]);
}

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

            nnue::m256_add_dpbusd_epi32(sum0, in, _mm256_load_si256(
                    reinterpret_cast<const __m256i*>(&weights[offset0 + j * register_width])));
            nnue::m256_add_dpbusd_epi32(sum1, in, _mm256_load_si256(
                    reinterpret_cast<const __m256i*>(&weights[offset1 + j * register_width])));
            nnue::m256_add_dpbusd_epi32(sum2, in, _mm256_load_si256(
                    reinterpret_cast<const __m256i*>(&weights[offset2 + j * register_width])));
            nnue::m256_add_dpbusd_epi32(sum3, in, _mm256_load_si256(
                    reinterpret_cast<const __m256i*>(&weights[offset3 + j * register_width])));
        }

        __m128i bias = _mm_load_si128((const __m128i*)&biases[i*4]);
        __m128i outval =  nnue::m256_haddx4(sum0, sum1, sum2, sum3, bias);

        outval = _mm_srai_epi32(outval, WEIGHT_SCALE_BITS);

        _mm_store_si128((__m128i*)&output[i*4], outval);
    }
}

void outputLayer(
    int32_t*           output,
    uint8_t*           input,
    const int8_t*      weights,
    const int32_t*     bias,
    const int          num_inputs) {

    constexpr uint32_t registerWidth = 256 / 8;

    __m256i sum = _mm256_setzero_si256();

    for (uint32_t j = 0; j < num_inputs; j += registerWidth)
    {
        const __m256i in = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + j));

        nnue::m256_add_dpbusd_epi32(sum, in, _mm256_load_si256(reinterpret_cast<const __m256i*>(weights + j)));
    }

    const __m128i sum128lo = _mm256_castsi256_si128(sum);
    const __m128i sum128hi = _mm256_extracti128_si256(sum, 1);

    const __m128i r4 = _mm_add_epi32(sum128lo, sum128hi);

    const __m128i r2 = _mm_add_epi32(r4, _mm_srli_si128(r4, 8));
    const __m128i r1 = _mm_add_epi32(r2, _mm_srli_si128(r2, 4));

    output[0] = _mm_cvtsi128_si32(r1) + bias[0];
}

void crelu(

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

// Evaluate a position using nnue
int nnueEval ( Side stm, GameInfo* gi ) {

    LayerData<layerSize[0], layerSize[0]> layerData;

    // INPUT LAYER 1
    // =============================================================================

     // TODO Use perspective after training with data that uses both perspectives
    //auto perspective = stm == WHITE ? 0 : 1;

    int offset = layerSize[0] / 2;
    for (int i = 0; i < layerSize[0] / 2; i++) {

        layerData.internal[i] = gi->accumulator[WHITE][i];
        layerData.internal[offset + i] = gi->accumulator[BLACK][i];
    }

    // this function clips values within the range 0 to 127
    crelu (layerSize[0], layerData.external, layerData.internal);


    // HIDDEN LAYER 1
    // =============================================================================

    hiddenLayer(layerData.internal, layerData.external, secondLayer.weights,
                       secondLayer.biases, layerSize[0], layerSize[1]);

    // this function clips values within the range 0 to 127
    crelu (layerSize[1], layerData.external, layerData.internal);


    // HIDDEN LAYER 2
    // =============================================================================

    hiddenLayer(layerData.internal, layerData.external, thirdLayer.weights,
                       thirdLayer.biases, layerSize[1], layerSize[2]);

    crelu (layerSize[2], layerData.external, layerData.internal);


    // OUTPUT LAYER
    // =============================================================================

    outputLayer(layerData.internal, layerData.external, fourthLayer.weights,
               fourthLayer.biases, layerSize[3]);

    // as opposed to the previous layers output layer value is not clipped

    int score = layerData.internal[0] / WEIGHT_SCALE_OUT;


    //return score;

    return stm == WHITE ? score : - score;
}







// NNUE NETWORK LOADING AND PARSING

// TODO refactor
uint32_t read_uint32_le(char *buffer)
{
    uint32_t val;
    char  *p = (char*)&val;

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

// TODO refactor
static bool checkNetHeader(char **data)
{
    char  *iter = *data;
    uint32_t version;

    version = read_uint32_le(iter);
    iter += 4;

    *data = iter;

    //return version == NET_VERSION;

    return true;
}


static bool scaleNetData(char **data)
{
    float *iter = (float*)*data;
    int   k;
    int   l;


    // INPUT / FEATURE LAYER
    // ==================================================================
    for (k = 0; k < FEATURE_LAYER_SIZE; k++, iter++)
        firstLayer.biases[k] = *iter * INPUT_LAYER_SCALE_BIAS;

    for (k=0; k<FEATURE_LAYER_SIZE * TOTAL_FEATURES; k++, iter++)
        firstLayer.weights[k] = *iter * INPUT_LAYER_SCALE_WEIGHT;


    // HIDDEN LAYER 1
    // ==================================================================
    for (k=0; k<32; k++,iter++)
        secondLayer.biases[k] = *iter * HIDDEN_LAYER_SCALE_BIAS;

    for (k=0; k<32; k++)
        for (l=0; l<FEATURE_LAYER_SIZE * 2; l++,iter++)
            secondLayer.weights[k*(FEATURE_LAYER_SIZE * 2)+l]
                = *iter * HIDDEN_LAYER_SCALE_WEIGHT;


    // HIDDEN LAYER 2
    // ==================================================================
    for (k=0; k<32; k++,iter++)
        thirdLayer.biases[k] = *iter * HIDDEN_LAYER_SCALE_BIAS;

    for (k=0; k<32; k++)
        for (l=0; l<32; l++,iter++)
            thirdLayer.weights[k*32+l] = *iter * HIDDEN_LAYER_SCALE_WEIGHT;


    // OUTPUT LAYER
    // ==================================================================
    for (k=0; k<1; k++,iter++)
        fourthLayer.biases[k] = *iter * OUTPUT_LAYER_SCALE_BIAS;

    for (k=0; k<1; k++)
        for (l=0; l<32; l++,iter++)
            fourthLayer.weights[k*32+l] = *iter * OUTPUT_LAYER_SCALE_WEIGHT;

    *data = (char*)iter;

    return true;
}

static uint32_t getNetSize() {

    uint32_t size = 0;
    int      k;

    size += NET_HEADER_SIZE;

    size += FEATURE_LAYER_SIZE*sizeof(float);
    size += FEATURE_LAYER_SIZE*TOTAL_FEATURES*sizeof(float);

    for (k=1;k<NUM_LAYERS;k++) {
        size += layerSize[k]*sizeof(float);
        size += layerSize[k]*layerSize[k-1]*sizeof(float);
    }

    return size;
}

static uint32_t readNetData (std::string path, char** data) {

    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (!file.is_open())
        return 0;

    file.ignore( std::numeric_limits<std::streamsize>::max());
    std::streamsize length = file.gcount();
    file.clear();   //  Since ignore will have set eof.
    file.seekg( 0, std::ios_base::beg );

    *data = new char[length];

    file.read( *data, length );

    const auto size = file.gcount();

    file.close();

    return size;
}

bool loadNetwork() {

    char *data = nullptr;

    const auto path1 = "/home/epoch206.nnue";
    const auto path2 = "epoch206.nnue";

    const std::string path =
        nnue::exists_file(path1) ? path1 :
        nnue::exists_file(path2) ? path2 : "";

    return !path.empty()
        && readNetData(path, &data) == getNetSize()
        && checkNetHeader(&data)
        && scaleNetData(&data);
}



