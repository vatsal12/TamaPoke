#pragma once
typedef int gpio_num_t;
typedef enum { GPIO_INTR_DISABLE = 0, GPIO_INTR_POSEDGE, GPIO_INTR_NEGEDGE,
               GPIO_INTR_ANYEDGE, GPIO_INTR_LOW_LEVEL, GPIO_INTR_HIGH_LEVEL } gpio_int_type_t;
inline int gpio_wakeup_enable(gpio_num_t, gpio_int_type_t) { return 0; }
inline int gpio_wakeup_disable(gpio_num_t) { return 0; }
inline int gpio_set_intr_type(gpio_num_t, gpio_int_type_t) { return 0; }
