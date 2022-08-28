#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#include "density_api.h"

#if defined(_WIN64) || defined(_WIN32)
#define DLLEXPORT __declspec(dllexport)
#elif
#define DLLEXPORT
#endif /* _MSC_VER */

const char *format_state(DENSITY_STATE state)
{
    if (state == DENSITY_STATE_ERROR_INPUT_BUFFER_TOO_SMALL)
    {
        return "input buffer too small";
    }
    else if (state == DENSITY_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL)
    {
        return "output buffer too small";
    }
    else if (state == DENSITY_STATE_ERROR_DURING_PROCESSING)
    {
        return "error during processing";
    }
    else if (state == DENSITY_STATE_ERROR_INVALID_CONTEXT)
    {
        return "invalid context";
    }
    else if (state == DENSITY_STATE_ERROR_INVALID_ALGORITHM)
    {
        return "invalid algorithm";
    }
    return NULL;
}


typedef struct ldensity_compressor_s
{
    DENSITY_STATE state;
    density_context *context;
} ldensity_compressor_t;


int ldensity_compressor_compress(lua_State *L)
{
    if (lua_gettop(L) != 2)
    {
        return luaL_error(L, "must have block to compress");
    }
    ldensity_compressor_t *self = (ldensity_compressor_t *) luaL_checkudata(L, 1, "density.Compressor");
    size_t len; // input size
    const char *block = luaL_checklstring(L, 2, &len);
    uint_fast64_t out_size = density_compress_safe_size((uint_fast64_t) len);
    void *out = lua_newuserdata(L, (size_t) out_size);
    density_processing_result result = density_compress_with_context((const uint8_t *) block, (const uint_fast64_t) len,
                                                                     (uint8_t *) out, (const uint_fast64_t) out_size,
                                                                     self->context);
    if (result.state != DENSITY_STATE_OK)
    {
        return luaL_error(L, format_state(result.state));
    }
    lua_pushlstring(L, (const char *) out, result.bytesWritten);
    return 1;
}

static int
ldensity_compressor_gc(lua_State *L)
{
    ldensity_compressor_t *self = (ldensity_compressor_t *) luaL_checkudata(L, 1, "density.Compressor");
    density_free_context(self->context, NULL);
    return 0;
}

static luaL_Reg ldensity_compressor_methods[] = {
        {"compress", &ldensity_compressor_compress},
        {"__gc",     &ldensity_compressor_gc},
        {NULL, NULL}
};


static int
lnewcompressor(lua_State *L)
{
    if (lua_gettop(L) != 2)
    {
        return luaL_error(L, "must have buffer and custom_dictionary");
    }
    DENSITY_ALGORITHM algorithm = luaL_checkinteger(L, 1);
    bool custom_dictionary = (bool) lua_toboolean(L, 2);
    density_processing_result result = density_compress_prepare_context(algorithm, custom_dictionary,
                                                                        NULL);
    ldensity_compressor_t *self = (ldensity_compressor_t *) lua_newuserdata(L, sizeof(ldensity_compressor_t));
    self->state = result.state;
    self->context = result.context;
    luaL_setmetatable(L, "density.Compressor");
    return 1;
}

typedef struct ldensity_decompressor_s
{
    DENSITY_STATE state;
    density_context *context;
} ldensity_decompressor_t;

static int
lnewdecompressor(lua_State *L)  // todo buffer必须在生命周期内保持有效
{
    if (lua_gettop(L) != 2)
    {
        return luaL_error(L, "must have buffer and custom_dictionary");
    }
    size_t len;
    const char *buffer = luaL_checklstring(L, 1, &len);
    bool custom_dictionary = (bool) lua_toboolean(L, 2);
    density_processing_result result = density_decompress_prepare_context((const uint8_t *) buffer,
                                                                          (const uint_fast64_t) len,
                                                                          custom_dictionary,
                                                                          NULL);

    ldensity_decompressor_t *self = (ldensity_decompressor_t *) lua_newuserdata(L, sizeof(ldensity_decompressor_t));
    self->state = result.state;
    self->context = result.context;
    luaL_setmetatable(L, "density.DeCompressor");
    return 1;
}

static int
ldensity_decompressor_decompress(lua_State *L)
{
    if (lua_gettop(L) != 3)
    {
        return luaL_error(L, "must have block and decompress_safe_size to decompress");
    }
    ldensity_decompressor_t *self = (ldensity_decompressor_t *) luaL_checkudata(L, 1, "density.DeCompressor");
    size_t len; // input size
    const char *block = luaL_checklstring(L, 2, &len);
    uint_fast64_t decompress_safe_size = (uint_fast64_t) luaL_checkinteger(L, 3);
    void *out = lua_newuserdata(L, (size_t) decompress_safe_size);
    density_processing_result result = density_decompress_with_context((const uint8_t *) block,
                                                                       (const uint_fast64_t) len, (uint8_t *) out,
                                                                       decompress_safe_size, self->context);
    if (result.state != DENSITY_STATE_OK)
    {
        return luaL_error(L, format_state(result.state));
    }
    lua_pushlstring(L, (const char *) out, result.bytesWritten);
    return 1;
}

static int
ldensity_decompressor_gc(lua_State *L)
{
    ldensity_decompressor_t *self = (ldensity_decompressor_t *) luaL_checkudata(L, 1, "density.DeCompressor");
    density_free_context(self->context, NULL);
    return 0;
}

static luaL_Reg ldensity_decompressor_methods[] = {
        {"decompress", &ldensity_decompressor_decompress},
        {"__gc",       &ldensity_decompressor_gc},
        {NULL, NULL}
};


static int ldensity_compress(lua_State *L)
{
    if (lua_gettop(L) != 2)
    {
        return luaL_error(L, "must have a data and algorithm to compress");
    }
    size_t len;
    const char *buffer = luaL_checklstring(L, 1, &len);
    DENSITY_ALGORITHM algorithm = luaL_checkinteger(L, 2);
    uint_fast64_t compress_safe_size = density_compress_safe_size((const uint_fast64_t) len);

    void *out = lua_newuserdata(L, (size_t) compress_safe_size);
    density_processing_result result = density_compress((const uint8_t *) buffer, (const uint_fast64_t) len,
                                                        (uint8_t *) out, compress_safe_size, algorithm);
    if (result.state != DENSITY_STATE_OK)
    {
        return luaL_error(L, format_state(result.state));
    }
    lua_pushlstring(L, (const char *) out, result.bytesWritten);
    return 1;
}

static int ldensity_decompress(lua_State *L)
{
    if (lua_gettop(L) != 2)
    {
        return luaL_error(L, "must have a data and decompress_safe_size to compress");
    }
    size_t len;
    const char *buffer = luaL_checklstring(L, 1, &len);
    uint_fast64_t decompress_safe_size = (uint_fast64_t) luaL_checkinteger(L, 2);
    void *out = lua_newuserdata(L, (size_t) decompress_safe_size);
    density_processing_result result = density_decompress((const uint8_t *) buffer, (const uint_fast64_t) len,
                                                          (uint8_t *) out, decompress_safe_size);
    if (result.state != DENSITY_STATE_OK)
    {
        return luaL_error(L, format_state(result.state));
    }
    lua_pushlstring(L, (const char *) out, result.bytesWritten);
    return 1;

}

static int ldensity_major_version(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer) density_version_major());
    return 1;
}

static int ldensity_minor_version(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer) density_version_minor());
    return 1;
}

static int ldensity_revision_version(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer) density_version_revision());
    return 1;
}

static int ldensity_get_dictionary_size(lua_State *L)
{
    if (lua_gettop(L) != 1)
    {
        return luaL_error(L, "algorithm is needed");
    }
    DENSITY_ALGORITHM algorithm = luaL_checkinteger(L, 1);
    lua_pushinteger(L, (lua_Integer) density_get_dictionary_size(algorithm));
    return 1;
}

static int ldensity_compress_safe_size(lua_State *L)
{
    if (lua_gettop(L) != 1)
    {
        return luaL_error(L, "input size is needed");
    }
    const uint_fast64_t input_size = (const uint_fast64_t) luaL_checkinteger(L, 1);
    lua_pushinteger(L, (lua_Integer) density_compress_safe_size(input_size));
    return 1;
}

static int ldensity_decompress_safe_size(lua_State *L)
{
    if (lua_gettop(L) != 1)
    {
        return luaL_error(L, "input size is needed");
    }
    const uint_fast64_t input_size = (const uint_fast64_t) luaL_checkinteger(L, 1);
    lua_pushinteger(L, (lua_Integer) density_decompress_safe_size(input_size));
    return 1;
}

// module level funcs
static luaL_Reg lua_funcs[] = {
        {"newcompressor", &lnewcompressor},
        {"newdecompressor", &lnewdecompressor},
        {"compress", &ldensity_compress},
        {"decompress", &ldensity_decompress},
        {"major_version", &ldensity_major_version},
        {"minor_version", &ldensity_minor_version},
        {"revision_version", &ldensity_revision_version},
        {"get_dictionary_size", &ldensity_get_dictionary_size},
        {"compress_safe_size", &ldensity_compress_safe_size},
        {"decompress_safe_size", &ldensity_decompress_safe_size},
        {NULL, NULL}
};

DLLEXPORT int luaopen_density(lua_State *L)
{
    if (!luaL_newmetatable(L, "density.Compressor"))
    {
        return luaL_error(L, "density.Compressor already in register");
    }
    lua_pushvalue(L, -1); // mt1 mt1
    lua_setfield(L, -2, "__index"); // mt1
    luaL_setfuncs(L, ldensity_compressor_methods, 0); // mt1

    if (!luaL_newmetatable(L, "density.DeCompressor"))
    {
        return luaL_error(L, "density.DeCompressor already in register");
    }
    lua_pushvalue(L, -1); // mt1 mt2 mt2
    lua_setfield(L, -2, "__index"); // mt1 mt2
    luaL_setfuncs(L, ldensity_decompressor_methods, 0); // mt

    luaL_newlib(L, lua_funcs);

#define lua_addintconstant(index, key, value) lua_pushinteger(L, value); \
                       lua_setfield(L, index, key);

    lua_addintconstant(-2, "DENSITY_ALGORITHM_CHAMELEON", DENSITY_ALGORITHM_CHAMELEON)
    lua_addintconstant(-2, "DENSITY_ALGORITHM_CHEETAH", DENSITY_ALGORITHM_CHEETAH)
    lua_addintconstant(-2, "DENSITY_ALGORITHM_LION", DENSITY_ALGORITHM_LION)

    lua_addintconstant(-2, "DENSITY_STATE_OK", DENSITY_STATE_OK)
    lua_addintconstant(-2, "DENSITY_STATE_ERROR_INPUT_BUFFER_TOO_SMALL", DENSITY_STATE_ERROR_INPUT_BUFFER_TOO_SMALL)
    lua_addintconstant(-2, "DENSITY_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL", DENSITY_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL)
    lua_addintconstant(-2, "DENSITY_STATE_ERROR_DURING_PROCESSING", DENSITY_STATE_ERROR_DURING_PROCESSING)
    lua_addintconstant(-2, "DENSITY_STATE_ERROR_INVALID_CONTEXT", DENSITY_STATE_ERROR_INVALID_CONTEXT)
    lua_addintconstant(-2, "DENSITY_STATE_ERROR_INVALID_ALGORITHM", DENSITY_STATE_ERROR_INVALID_ALGORITHM)
    return 1;
}