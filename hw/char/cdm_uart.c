/*
 * Logisim-compatible CdM UART: byte-wide flags and data registers.
 */

#include "qemu/osdep.h"
#include "chardev/char-fe.h"
#include "hw/char/cdm_uart.h"
#include "hw/core/qdev-properties-system.h"
#include "hw/core/sysbus.h"
#include "migration/vmstate.h"
#include "qemu/fifo8.h"
#include "qemu/module.h"

#define CDM_UART_CONNECTED 0x01
#define CDM_UART_HAS_DATA  0x02
#define CDM_UART_RX_SIZE   4096

struct CdmUartState {
    SysBusDevice parent_obj;
    MemoryRegion flags;
    MemoryRegion data;
    CharFrontend chr;
    Fifo8 rx;
    GByteArray *tx;
    guint tx_watch;
};

static gboolean cdm_uart_transmit(void *channel, GIOCondition cond,
                                  void *opaque);

static void cdm_uart_clear_tx(CdmUartState *s) {
    g_clear_handle_id(&s->tx_watch, g_source_remove);
    g_byte_array_set_size(s->tx, 0);
}

static void cdm_uart_flush(CdmUartState *s) {
    int written;

    if (!qemu_chr_fe_backend_open(&s->chr)) {
        cdm_uart_clear_tx(s);
        return;
    }

    while (s->tx->len) {
        written = qemu_chr_fe_write(&s->chr, s->tx->data,
                                    MIN(s->tx->len, INT_MAX));
        if (written <= 0) {
            if (!s->tx_watch) {
                s->tx_watch = qemu_chr_fe_add_watch(&s->chr,
                    G_IO_OUT | G_IO_HUP, cdm_uart_transmit, s);
            }
            /* Бэкенд без возможности записи не может откладывать вывод. */
            if (!s->tx_watch) {
                g_byte_array_set_size(s->tx, 0);
            }
            return;
        }
        g_byte_array_remove_range(s->tx, 0, written);
    }
    g_clear_handle_id(&s->tx_watch, g_source_remove);
}

static gboolean cdm_uart_transmit(void *channel, GIOCondition cond,
                                  void *opaque) {
    CdmUartState *s = opaque;

    s->tx_watch = 0;
    if (cond & G_IO_HUP) {
        g_byte_array_set_size(s->tx, 0);
    } else {
        cdm_uart_flush(s);
    }
    return G_SOURCE_REMOVE;
}

static uint64_t cdm_uart_flags_read(void *opaque, hwaddr offset, unsigned size) {
    CdmUartState *s = opaque;

    return (qemu_chr_fe_backend_open(&s->chr) ? CDM_UART_CONNECTED : 0) |
           (!fifo8_is_empty(&s->rx) ? CDM_UART_HAS_DATA : 0);
}

static void cdm_uart_flags_write(void *opaque, hwaddr offset,
                                 uint64_t value, unsigned size) {
    /* Регистр флагов доступен только для чтения. */
}

static uint64_t cdm_uart_data_read(void *opaque, hwaddr offset, unsigned size) {
    CdmUartState *s = opaque;
    uint8_t value = 0;

    if (!fifo8_is_empty(&s->rx)) {
        value = fifo8_pop(&s->rx);
        qemu_chr_fe_accept_input(&s->chr);
    }
    return value;
}

static void cdm_uart_data_write(void *opaque, hwaddr offset,
                                uint64_t value, unsigned size) {
    CdmUartState *s = opaque;
    uint8_t byte = value;

    if (qemu_chr_fe_backend_open(&s->chr)) {
        g_byte_array_append(s->tx, &byte, 1);
        if (!s->tx_watch) {
            cdm_uart_flush(s);
        }
    }
}

static const MemoryRegionOps cdm_uart_flags_ops = {
    .read = cdm_uart_flags_read,
    .write = cdm_uart_flags_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = { .min_access_size = 1, .max_access_size = 1 },
    .impl = { .min_access_size = 1, .max_access_size = 1 },
};

static const MemoryRegionOps cdm_uart_data_ops = {
    .read = cdm_uart_data_read,
    .write = cdm_uart_data_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = { .min_access_size = 1, .max_access_size = 1 },
    .impl = { .min_access_size = 1, .max_access_size = 1 },
};

static int cdm_uart_can_receive(void *opaque) {
    CdmUartState *s = opaque;

    return fifo8_num_free(&s->rx);
}

static void cdm_uart_receive(void *opaque, const uint8_t *buf, int size) {
    CdmUartState *s = opaque;

    assert(size <= fifo8_num_free(&s->rx));
    fifo8_push_all(&s->rx, buf, size);
}

static void cdm_uart_event(void *opaque, QEMUChrEvent event) {
    CdmUartState *s = opaque;

    if (event == CHR_EVENT_CLOSED) {
        cdm_uart_clear_tx(s);
    }
}

static void cdm_uart_reset(DeviceState *dev) {
    CdmUartState *s = CDM_UART(dev);

    fifo8_reset(&s->rx);
    cdm_uart_clear_tx(s);
    qemu_chr_fe_accept_input(&s->chr);
}

static void cdm_uart_realize(DeviceState *dev, Error **errp) {
    CdmUartState *s = CDM_UART(dev);

    qemu_chr_fe_set_handlers(&s->chr, cdm_uart_can_receive, cdm_uart_receive,
                             cdm_uart_event, NULL, s, NULL, true);
}

static void cdm_uart_unrealize(DeviceState *dev) {
    CdmUartState *s = CDM_UART(dev);

    cdm_uart_clear_tx(s);
    qemu_chr_fe_deinit(&s->chr, false);
}

static int cdm_uart_post_load(void *opaque, int version_id) {
    CdmUartState *s = opaque;

    cdm_uart_flush(s);
    qemu_chr_fe_accept_input(&s->chr);
    return 0;
}

static const VMStateDescription cdm_uart_vmstate = {
    .name = TYPE_CDM_UART,
    .version_id = 1,
    .minimum_version_id = 1,
    .post_load = cdm_uart_post_load,
    .fields = (const VMStateField[]) {
        VMSTATE_FIFO8(rx, CdmUartState),
        VMSTATE_GBYTEARRAY(tx, CdmUartState, 1),
        VMSTATE_END_OF_LIST()
    },
};

static void cdm_uart_init(Object *obj) {
    CdmUartState *s = CDM_UART(obj);

    fifo8_create(&s->rx, CDM_UART_RX_SIZE);
    s->tx = g_byte_array_new();
    memory_region_init_io(&s->flags, obj, &cdm_uart_flags_ops, s,
                          "cdm-uart.flags", 1);
    memory_region_init_io(&s->data, obj, &cdm_uart_data_ops, s,
                          "cdm-uart.data", 1);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->flags);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->data);
}

static void cdm_uart_finalize(Object *obj) {
    CdmUartState *s = CDM_UART(obj);

    fifo8_destroy(&s->rx);
    g_byte_array_unref(s->tx);
}

static const Property cdm_uart_properties[] = {
    DEFINE_PROP_CHR("chardev", CdmUartState, chr),
};

static void cdm_uart_class_init(ObjectClass *klass, const void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = cdm_uart_realize;
    dc->unrealize = cdm_uart_unrealize;
    dc->vmsd = &cdm_uart_vmstate;
    device_class_set_legacy_reset(dc, cdm_uart_reset);
    device_class_set_props(dc, cdm_uart_properties);
}

static const TypeInfo cdm_uart_info = {
    .name = TYPE_CDM_UART,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(CdmUartState),
    .instance_init = cdm_uart_init,
    .instance_finalize = cdm_uart_finalize,
    .class_init = cdm_uart_class_init,
};

static void cdm_uart_register_types(void) {
    type_register_static(&cdm_uart_info);
}

type_init(cdm_uart_register_types)