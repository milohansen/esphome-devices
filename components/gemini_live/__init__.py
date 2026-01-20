import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_MICROPHONE, CONF_SPEAKER, CONF_TRIGGER_ID
from esphome.components import microphone, speaker
from pathlib import Path
from esphome import automation

CODEOWNERS = ["@miloh"]
DEPENDENCIES = ["network"]
AUTO_LOAD = []

gemini_live_ns = cg.esphome_ns.namespace("gemini_live")
GeminiLiveComponent = gemini_live_ns.class_("GeminiLiveComponent", cg.Component)

CONF_ON_STOP_STREAMING = "on_stop_streaming"

GeminiLiveOnStopStreamingTrigger = gemini_live_ns.class_(
    "GeminiLiveOnStopStreamingTrigger", automation.Trigger
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(GeminiLiveComponent),
        cv.Required(CONF_MICROPHONE): cv.use_id(microphone.Microphone),
        cv.Required(CONF_SPEAKER): cv.use_id(speaker.Speaker),
        cv.Optional(CONF_ON_STOP_STREAMING): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                    GeminiLiveOnStopStreamingTrigger
                ),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    # cg.add_global(cg.include("gemini_live.h"))
    component_dir = Path(__file__).parent
    cg.add_build_flag(f"-I{component_dir}")

    mic = await cg.get_variable(config[CONF_MICROPHONE])
    spk = await cg.get_variable(config[CONF_SPEAKER])

    var = cg.new_Pvariable(config[CONF_ID], mic, spk)
    await cg.register_component(var, config)

    # 4. Register the triggers
    for conf in config.get(CONF_ON_STOP_STREAMING, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        # cg.add(var.register_stop_trigger(trigger))
        await automation.build_automation(trigger, [], conf)
