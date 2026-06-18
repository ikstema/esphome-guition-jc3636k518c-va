"""ESPHome component for DRV2605 Haptic Motor Driver."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome import automation
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]
CODEOWNERS = ["@RAR"]

drv2605_ns = cg.esphome_ns.namespace("drv2605")
DRV2605Component = drv2605_ns.class_("DRV2605Component", cg.Component, i2c.I2CDevice)

# Actions
PlayEffectAction = drv2605_ns.class_("PlayEffectAction", automation.Action)
PlaySequenceAction = drv2605_ns.class_("PlaySequenceAction", automation.Action)
StopAction = drv2605_ns.class_("StopAction", automation.Action)
SetRealtimeValueAction = drv2605_ns.class_("SetRealtimeValueAction", automation.Action)
StandbyAction = drv2605_ns.class_("StandbyAction", automation.Action)
WakeAction = drv2605_ns.class_("WakeAction", automation.Action)

# Motor types
CONF_MOTOR_TYPE = "motor_type"
MOTOR_TYPE_ERM = "ERM"
MOTOR_TYPE_LRA = "LRA"

# Library selection for ERM motors
CONF_LIBRARY = "library"
LIBRARY_EMPTY = 0
LIBRARY_TS2200_LIBRARY_A = 1
LIBRARY_TS2200_LIBRARY_B = 2
LIBRARY_TS2200_LIBRARY_C = 3
LIBRARY_TS2200_LIBRARY_D = 4
LIBRARY_TS2200_LIBRARY_E = 5
LIBRARY_LRA = 6

# Action configuration keys
CONF_EFFECT = "effect"
CONF_EFFECTS = "effects"
CONF_VALUE = "value"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(DRV2605Component),
        cv.Optional(CONF_MOTOR_TYPE, default=MOTOR_TYPE_ERM): cv.enum(
            {MOTOR_TYPE_ERM: MOTOR_TYPE_ERM, MOTOR_TYPE_LRA: MOTOR_TYPE_LRA},
            upper=True,
        ),
        cv.Optional(CONF_LIBRARY, default=1): cv.int_range(min=0, max=6),
    }
).extend(cv.COMPONENT_SCHEMA).extend(i2c.i2c_device_schema(0x5A))


async def to_code(config):
    """Code generation for DRV2605 component."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    motor_type_is_lra = config[CONF_MOTOR_TYPE] == MOTOR_TYPE_LRA
    cg.add(var.set_motor_type(motor_type_is_lra))
    cg.add(var.set_library(config[CONF_LIBRARY]))


# Action schemas
@automation.register_action(
    "drv2605.play_effect",
    PlayEffectAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DRV2605Component),
            cv.Required(CONF_EFFECT): cv.templatable(cv.int_range(min=1, max=123)),
        }
    ),
    synchronous=True,
)
async def play_effect_to_code(config, action_id, template_arg, args):
    """Code generation for play_effect action."""
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    
    template_ = await cg.templatable(config[CONF_EFFECT], args, cg.uint8)
    cg.add(var.set_effect(template_))
    return var


@automation.register_action(
    "drv2605.play_sequence",
    PlaySequenceAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DRV2605Component),
            cv.Required(CONF_EFFECTS): cv.All(
                cv.ensure_list(cv.int_range(min=1, max=123)),
                cv.Length(min=1, max=8),
            ),
        }
    ),
    synchronous=True,
)
async def play_sequence_to_code(config, action_id, template_arg, args):
    """Code generation for play_sequence action."""
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    cg.add(var.set_effects(config[CONF_EFFECTS]))
    return var


@automation.register_action(
    "drv2605.stop",
    StopAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DRV2605Component),
        }
    ),
    synchronous=True,
)
async def stop_to_code(config, action_id, template_arg, args):
    """Code generation for stop action."""
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var


@automation.register_action(
    "drv2605.set_realtime_value",
    SetRealtimeValueAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DRV2605Component),
            cv.Required(CONF_VALUE): cv.templatable(cv.int_range(min=0, max=255)),
        }
    ),
    synchronous=True,
)
async def set_realtime_value_to_code(config, action_id, template_arg, args):
    """Code generation for set_realtime_value action."""
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    
    template_ = await cg.templatable(config[CONF_VALUE], args, cg.uint8)
    cg.add(var.set_value(template_))
    return var


@automation.register_action(
    "drv2605.standby",
    StandbyAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DRV2605Component),
        }
    ),
    synchronous=True,
)
async def standby_to_code(config, action_id, template_arg, args):
    """Code generation for standby action (low-power mode)."""
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var


@automation.register_action(
    "drv2605.wake",
    WakeAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DRV2605Component),
        }
    ),
    synchronous=True,
)
async def wake_to_code(config, action_id, template_arg, args):
    """Code generation for wake action (exit standby mode)."""
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var

