#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "st1vafe3bx_reg.h"

typedef struct {
  uint8_t regs[256];
  uint8_t writes[8];
  unsigned int count;
  int fail_reg;
} fake_bus_t;

static int32_t fake_read(void *handle, uint8_t reg, uint8_t *data, uint16_t len)
{
  fake_bus_t *bus = (fake_bus_t *)handle;
  memcpy(data, &bus->regs[reg], len);
  return 0;
}

static int32_t fake_write(void *handle, uint8_t reg, const uint8_t *data, uint16_t len)
{
  fake_bus_t *bus = (fake_bus_t *)handle;
  assert(bus->count < sizeof(bus->writes));
  bus->writes[bus->count++] = reg;
  if (reg == bus->fail_reg)
  {
    return -1;
  }
  memcpy(&bus->regs[reg], data, len);
  return 0;
}

static stmdev_ctx_t context(fake_bus_t *bus)
{
  stmdev_ctx_t ctx = {0};
  ctx.read_reg = fake_read;
  ctx.write_reg = fake_write;
  ctx.handle = bus;
  return ctx;
}

int main(void)
{
  fake_bus_t bus = {0};
  stmdev_ctx_t ctx = context(&bus);
  st1vafe3bx_fifo_mode_t mode = {0};
  bus.fail_reg = -1;
  bus.regs[ST1VAFE3BX_CTRL4] = 0xA1U;
  mode.operation = ST1VAFE3BX_STREAM_MODE;
  assert(st1vafe3bx_fifo_mode_set(&ctx, mode) == 0);
  assert(bus.count == 4);
  assert(bus.writes[2] == ST1VAFE3BX_CTRL4);
  assert(bus.writes[3] == ST1VAFE3BX_FIFO_CTRL);
  assert(bus.regs[ST1VAFE3BX_CTRL4] == 0xA9U);
  assert((bus.regs[ST1VAFE3BX_FIFO_CTRL] & 0x07U) == ST1VAFE3BX_STREAM_MODE);

  memset(&bus, 0, sizeof(bus));
  bus.fail_reg = ST1VAFE3BX_CTRL4;
  assert(st1vafe3bx_fifo_mode_set(&ctx, mode) != 0);
  assert(bus.count == 3);
  assert(bus.writes[2] == ST1VAFE3BX_CTRL4);

  memset(&bus, 0, sizeof(bus));
  bus.fail_reg = ST1VAFE3BX_FIFO_WTM;
  assert(st1vafe3bx_fifo_mode_set(&ctx, mode) != 0);
  assert(bus.count == 2);

  memset(&bus, 0, sizeof(bus));
  bus.fail_reg = ST1VAFE3BX_FIFO_BATCH_DEC;
  assert(st1vafe3bx_fifo_mode_set(&ctx, mode) != 0);
  assert(bus.count == 1);

  memset(&bus, 0, sizeof(bus));
  bus.fail_reg = -1;
  bus.regs[ST1VAFE3BX_CTRL4] = 0xFF;
  mode.operation = ST1VAFE3BX_FIFO_OFF;
  assert(st1vafe3bx_fifo_mode_set(&ctx, mode) == 0);
  assert(bus.count == 4);
  assert(bus.writes[2] == ST1VAFE3BX_FIFO_CTRL);
  assert(bus.writes[3] == ST1VAFE3BX_CTRL4);
  assert((bus.regs[ST1VAFE3BX_CTRL4] & 0x08U) == 0);
  puts("FIFO mode tests passed");
  return 0;
}
