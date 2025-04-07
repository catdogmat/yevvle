
#include "soc/io_mux_reg.h"

// Deep Sleep pin controls
#define GPIO_MODE_INPUT(pin) \
  PIN_FUNC_SELECT(GPIO_PIN_REG_##pin, PIN_FUNC_GPIO); \
  PIN_INPUT_ENABLE(GPIO_PIN_REG_##pin); \
  REG_WRITE(GPIO_ENABLE_W1TC_REG, (1UL << pin));

#define GPIO_MODE_OUTPUT(pin) \
  PIN_FUNC_SELECT(GPIO_PIN_REG_##pin, PIN_FUNC_GPIO); \
  PIN_INPUT_DISABLE(GPIO_PIN_REG_##pin); \
  REG_WRITE(GPIO_ENABLE_W1TC_REG, (1UL << pin));
  
#if(HW_VERSION >= 10)
  // Set wakeup timer when we guess display will finish refreshing, to put display to hibernation
  #define GPIO_OUTPUT_SET(gpio_no, bit_value) gpio_output_set(bit_value<<gpio_no, (bit_value ? 0 : 1)<<gpio_no, 1<<gpio_no,0)
  #define GPIO_DIS_OUTPUT(gpio_no)   (gpio_output_set(0,0,0, 1<<gpio_no))
  #define GPIO_INPUT_GET(gpio_no)    ((gpio_input_get()>>gpio_no)&BIT0)
#endif