import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components import microphone, speaker

DEPENDENCIES = ['network']

gemini_live_ns = cg.esphome_ns.namespace('gemini_live')
GeminiLiveComponent = gemini_live_ns.class_('GeminiLiveComponent', cg.Component)

CONF_MICROPHONE = "microphone"
CONF_SPEAKER = "speaker"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(GeminiLiveComponent),
    cv.Required(CONF_MICROPHONE): cv.use_id(microphone.Microphone),
    cv.Required(CONF_SPEAKER): cv.use_id(speaker.Speaker),
})

async def to_code(config):
    cg.add_global(cg.include("gemini_live.h"))
    mic = await cg.get_variable(config[CONF_MICROPHONE])
    spk = await cg.get_variable(config[CONF_SPEAKER])
    
    var = cg.new_Pvariable(config[CONF_ID], mic, spk)
    await cg.register_component(var, config)
