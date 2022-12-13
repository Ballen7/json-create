#include "sjson.h"

#define SJSON_WRITE_ADDRESS(ctx) ((char *)&ctx->pBuf[ctx->index])
#define SJSON_AVAIABLE_SPACE(ctx) ((size_t)(ctx->buf_size - 1) - ctx->index)

static const uint8_t *const SJSON_TRUE_STRING = (const uint8_t *)"true";
static const uint8_t *const SJSON_FALSE_STRING = (const uint8_t *)"false";

static sjson_retval_t sjson_valid_context(sjson_context_t *ctx) {
  if (ctx->pBuf == NULL) return SJSON_ERROR;
  if (ctx->buf_size == 0) return SJSON_ERROR;
  if (SJSON_AVAIABLE_SPACE(ctx) == 0) return SJSON_ERROR;
  if (ctx->state != SJSON_NEXT_KEY_PAIR && ctx->state != SJSON_START)
    return SJSON_ERROR;

  if (ctx->state == SJSON_NEXT_KEY_PAIR && SJSON_AVAIABLE_SPACE(ctx))
    ctx->pBuf[ctx->index++] = ',';

  return SJSON_SUCCESS;
}

static sjson_retval_t sjson_add_key(sjson_context_t *ctx, uint8_t *key,
                                    size_t key_len) {
  if (SJSON_AVAIABLE_SPACE(ctx) < 1 + key_len + 1 + 1) return SJSON_ERROR;

  ctx->pBuf[ctx->index++] = '"';
  memcpy(&ctx->pBuf[ctx->index], (void *)key, key_len);
  ctx->index += key_len;
  ctx->pBuf[ctx->index++] = '"';
  ctx->pBuf[ctx->index++] = ':';

  return SJSON_SUCCESS;
}

static sjson_retval_t int8_handler(sjson_context_t *ctx, void *value) {
  int32_t space_used;
  uint8_t _val = (uint8_t) * (uint8_t *)value;
  space_used = (int32_t)SJSON_SNPRINTF(SJSON_WRITE_ADDRESS(ctx),
                                       SJSON_AVAIABLE_SPACE(ctx), "%u", _val);
  ctx->index += space_used;
  return (ctx->index > ctx->buf_size) ? SJSON_ERROR : SJSON_SUCCESS;
}

static sjson_retval_t int16_handler(sjson_context_t *ctx, void *value) {
  int32_t space_used;
  uint16_t _val = (uint16_t) * (uint16_t *)value;
  space_used = (int32_t)SJSON_SNPRINTF(SJSON_WRITE_ADDRESS(ctx),
                                       SJSON_AVAIABLE_SPACE(ctx), "%u", _val);
  ctx->index += space_used;
  return (ctx->index > ctx->buf_size) ? SJSON_ERROR : SJSON_SUCCESS;
}

static sjson_retval_t int32_handler(sjson_context_t *ctx, void *value) {
  int32_t space_used;
  uint32_t _val = (uint32_t) * (uint32_t *)value;
  space_used = (int32_t)SJSON_SNPRINTF(SJSON_WRITE_ADDRESS(ctx),
                                       SJSON_AVAIABLE_SPACE(ctx), "%u", _val);
  ctx->index += space_used;
  return (ctx->index > ctx->buf_size) ? SJSON_ERROR : SJSON_SUCCESS;
}

static sjson_retval_t int64_handler(sjson_context_t *ctx, void *value) {
  int32_t space_used;
  uint64_t _val = (uint64_t) * (uint64_t *)value;
  space_used = (int32_t)SJSON_SNPRINTF(SJSON_WRITE_ADDRESS(ctx),
                                       SJSON_AVAIABLE_SPACE(ctx), "%lu", _val);
  ctx->index += space_used;
  return (ctx->index > ctx->buf_size) ? SJSON_ERROR : SJSON_SUCCESS;
}

static sjson_retval_t (*sjson_add_value[SJSON_INT_MAX_INDEX])(
    sjson_context_t *ctx, void *value) = {int8_handler, int16_handler,
                                          int32_handler, int64_handler};

sjson_retval_t sjson_add_integer(sjson_context_t *ctx, uint8_t *key,
                                 size_t key_len, void *value,
                                 sjson_integer_size_t type) {
  if (type > SJSON_INT_MAX_INDEX - 1) return SJSON_ERROR;
  if (SJSON_SUCCESS != sjson_valid_context(ctx)) return SJSON_ERROR;

  ctx->state = SJSON_IN_PROGRESS;
  if (SJSON_SUCCESS != sjson_add_key(ctx, key, key_len)) return SJSON_ERROR;
  if (SJSON_SUCCESS != sjson_add_value[type](ctx, value)) return SJSON_ERROR;
  ctx->state = SJSON_NEXT_KEY_PAIR;

  return SJSON_SUCCESS;
}

sjson_retval_t sjson_add_string(sjson_context_t *ctx, uint8_t *key,
                                size_t key_len, void *value, size_t value_len) {
  if (SJSON_SUCCESS != sjson_valid_context(ctx)) return SJSON_ERROR;

  ctx->state = SJSON_IN_PROGRESS;

  if (SJSON_SUCCESS != sjson_add_key(ctx, key, key_len)) return SJSON_ERROR;
  if (SJSON_AVAIABLE_SPACE(ctx) < 1 + value_len + 1) return SJSON_ERROR;

  ctx->pBuf[ctx->index++] = '"';
  memcpy(&ctx->pBuf[ctx->index], value, value_len);
  ctx->index += value_len;
  ctx->pBuf[ctx->index++] = '"';

  ctx->state = SJSON_NEXT_KEY_PAIR;

  return SJSON_SUCCESS;
}

sjson_retval_t sjson_add_boolean(sjson_context_t *ctx, uint8_t *key,
                                 size_t key_len, sjson_boolean_t bool_val) {
  const uint8_t *pStr;

  if (bool_val > SJSON_BOOLEAN_MAX - 1) return SJSON_ERROR;
  if (SJSON_SUCCESS != sjson_valid_context(ctx)) return SJSON_ERROR;

  ctx->state = SJSON_IN_PROGRESS;
  if (SJSON_SUCCESS != sjson_add_key(ctx, key, key_len)) return SJSON_ERROR;
  if (bool_val == SJSON_TRUE)
    pStr = SJSON_TRUE_STRING;
  else
    pStr = SJSON_FALSE_STRING;
  if (SJSON_AVAIABLE_SPACE(ctx) < strlen((const char *)pStr) + 1)
    return SJSON_ERROR;

  memcpy(&ctx->pBuf[ctx->index], pStr, strlen((const char *)pStr));
  ctx->index += strlen((const char *)pStr);
  ctx->state = SJSON_NEXT_KEY_PAIR;

  return SJSON_SUCCESS;
}

sjson_retval_t sjson_init(sjson_context_t *ctx, uint8_t *buf, size_t buf_size) {
  if (buf == NULL) return SJSON_INVALID_BUFFER;
  if (buf_size == 0) return SJSON_INVALID_BUFFER;

  memset(buf, 0, buf_size);
  memset(ctx, 0, sizeof(sjson_context_t));
  ctx->pBuf = buf;
  ctx->buf_size = buf_size;

  ctx->state = SJSON_START;
  if (SJSON_SUCCESS != sjson_valid_context(ctx)) return SJSON_ERROR;
  ctx->pBuf[ctx->index++] = '{';

  return SJSON_SUCCESS;
}

sjson_retval_t sjson_complete(sjson_context_t *ctx) {
  if (ctx->state != SJSON_NEXT_KEY_PAIR) return SJSON_ERROR;
  if (SJSON_AVAIABLE_SPACE(ctx) < 1) return SJSON_ERROR;

  ctx->pBuf[ctx->index++] = '}';
  ctx->state = SJSON_COMPLETE;
  sjson_log(">>%s<<\n\n", ctx->pBuf);

  return SJSON_SUCCESS;
}
