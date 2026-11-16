/*
 * CdM-16 debug memory access tests.
 */

#include "qemu/osdep.h"
#include "libqtest.h"

static void test_debug_memory(gconstpointer data) {
    bool harvard = GPOINTER_TO_INT(data);
    QTestState *qts = qtest_initf("-M cdm16,harvard=%s -S",
                                harvard ? "on" : "off");
    const struct {
        unsigned int addr;
        const char *von_neumann;
        const char *harvard;
    } translations[] = {
        { 0, "gpa: 0x0\r\n", "gpa: 0x10000\r\n" },
        { 0x1235, "gpa: 0x1235\r\n", "gpa: 0x11235\r\n" },
        { 0xffff, "gpa: 0xffff\r\n", "gpa: 0x1ffff\r\n" },
        { 0x11235, "gpa: 0x1235\r\n", "gpa: 0x11235\r\n" },
    };

    for (size_t i = 0; i < G_N_ELEMENTS(translations); i++) {
        g_autofree char *response = qtest_hmp(qts, "gva2gpa 0x%x",
                                            translations[i].addr);

        g_assert_cmpstr(response, ==, harvard ? translations[i].harvard :
                                              translations[i].von_neumann);
    }

    /* Тестирование работы с памятью */
    qtest_writeb(qts, 0x1235, 0x5a);
    qtest_writeb(qts, harvard ? 0x11235 : 0x1235, 0xa5);

    g_autofree char *response = qtest_hmp(qts, "x /1bx 0x1235");

    g_assert_nonnull(strstr(response, ": 0xa5"));
    qtest_quit(qts);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);

    qtest_add_data_func("debug-memory/von-neumann", GINT_TO_POINTER(false),
                        test_debug_memory);
    qtest_add_data_func("debug-memory/harvard", GINT_TO_POINTER(true),
                        test_debug_memory);

    return g_test_run();
}