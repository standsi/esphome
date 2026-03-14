import esphome.codegen as cg
from esphome.components.esp32 import (
    add_idf_sdkconfig_option,
    include_builtin_idf_component,
)
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32"]

lp_blink_ns = cg.esphome_ns.namespace("lp_blink")
LPBlinkComponent = lp_blink_ns.class_("LPBlinkComponent", cg.Component)

CONF_GPIO_NUM = "gpio_num"
CONF_INIT_STATE = "init_state"
CONF_PULSE_WIDTH_US = "pulse_width_us"
CONF_WAKE_PERIOD_MS = "wake_period_ms"

InitState = lp_blink_ns.enum("InitState")

INIT_STATE_OPTIONS = {
    "running": InitState.RUNNING,
    "stopped": InitState.STOPPED,
    "last": InitState.LAST,
}


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LPBlinkComponent),
        cv.Required(CONF_GPIO_NUM): cv.int_range(min=0, max=48),
        cv.Optional(CONF_INIT_STATE, default="running"): cv.enum(
            INIT_STATE_OPTIONS, lower=True
        ),
        cv.Optional(CONF_PULSE_WIDTH_US, default=30000): cv.positive_int,
        cv.Optional(CONF_WAKE_PERIOD_MS, default=1000): cv.positive_int,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    include_builtin_idf_component("ulp")
    add_idf_sdkconfig_option("CONFIG_ULP_COPROC_ENABLED", True)
    add_idf_sdkconfig_option("CONFIG_ULP_COPROC_TYPE_LP_CORE", True)
    add_idf_sdkconfig_option("CONFIG_ULP_COPROC_RESERVE_MEM", 4096)

    var = cg.new_Pvariable(config[CONF_ID], config[CONF_GPIO_NUM])
    await cg.register_component(var, config)
    cg.add(var.set_init_state(config[CONF_INIT_STATE]))
    cg.add(var.set_pulse_width_us(config[CONF_PULSE_WIDTH_US]))
    cg.add(var.set_wakeup_period_ms(config[CONF_WAKE_PERIOD_MS]))
