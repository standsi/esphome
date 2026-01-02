from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PIN

ulp_flash_ns = cg.esphome_ns.namespace("ulp_flash")
ULPFlash = ulp_flash_ns.class_("ULPFlash", cg.Component)
pulse_width_enum = ulp_flash_ns.enum("FlashPulseWidth", is_class=True)

PULSE_WIDTHS = {
    "narrow": pulse_width_enum.NARROW,
    "medium": pulse_width_enum.MEDIUM,
    "wide": pulse_width_enum.WIDE,
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ULPFlash),
        cv.Required(CONF_PIN): pins.internal_gpio_output_pin_schema,
        cv.Optional("interval", default="1s"): cv.positive_time_period_milliseconds,
        cv.Optional("pulse_width", default="narrow"): cv.enum(
            PULSE_WIDTHS,
            lower=True,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    pin = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pin(pin))
    cg.add(var.set_interval(config["interval"]))
    cg.add(var.set_pulse_width(config["pulse_width"]))
