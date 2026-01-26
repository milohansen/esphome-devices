import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_MICROPHONE,
    CONF_SPEAKER,
    CONF_TRIGGER_ID,
    CONF_MODEL,
)
from esphome.components import microphone, speaker, text_sensor
from pathlib import Path
from esphome import automation

CODEOWNERS = ["@miloh"]
DEPENDENCIES = ["network"]
AUTO_LOAD = ["text_sensor"]

gemini_live_ns = cg.esphome_ns.namespace("gemini_live")
GeminiLiveComponent = gemini_live_ns.class_("GeminiLiveComponent", cg.Component)

CONF_ON_STOP_STREAMING = "on_stop_streaming"
CONF_CONNECTION_TYPE = "connection_type"
CONF_GEMINI_TOKEN = "gemini_token"
CONF_GEMINI_CONFIG = "gemini_config"


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
        cv.Optional(CONF_CONNECTION_TYPE, "bridge"): cv.string, # type: ignore
        cv.Optional(
            CONF_MODEL, default="gemini-2.5-flash-native-audio-preview-12-2025" # type: ignore
        ): cv.string,
        cv.Optional(CONF_GEMINI_TOKEN): cv.use_id(text_sensor.TextSensor),
        cv.Optional(CONF_GEMINI_CONFIG): cv.use_id(text_sensor.TextSensor),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    component_dir = Path(__file__).parent
    cg.add_build_flag(f"-I{component_dir}")

    mic = await cg.get_variable(config[CONF_MICROPHONE])
    spk = await cg.get_variable(config[CONF_SPEAKER])

    var = cg.new_Pvariable(config[CONF_ID], mic, spk)
    await cg.register_component(var, config)

    cg.add(var.set_connection_type(config[CONF_CONNECTION_TYPE]))
    cg.add(var.set_model(config[CONF_MODEL]))

    if CONF_GEMINI_TOKEN in config:
        token_sensor = await cg.get_variable(config[CONF_GEMINI_TOKEN])
        cg.add(var.set_gemini_token_sensor(token_sensor))
    if CONF_GEMINI_CONFIG in config:
        config_sensor = await cg.get_variable(config[CONF_GEMINI_CONFIG])
        cg.add(var.set_gemini_config_sensor(config_sensor))

    for conf in config.get(CONF_ON_STOP_STREAMING, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
