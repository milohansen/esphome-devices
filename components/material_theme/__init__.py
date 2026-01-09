"""Material Theme component for ESPHome."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome import automation
from pathlib import Path

CODEOWNERS = ["@miloh"]
DEPENDENCIES = []
AUTO_LOAD = []

material_theme_ns = cg.esphome_ns.namespace("material_theme")
MaterialThemeComponent = material_theme_ns.class_("MaterialThemeComponent", cg.Component)

# Actions
GenerateSchemeAction = material_theme_ns.class_("GenerateSchemeAction", automation.Action)
ApplySchemeAction = material_theme_ns.class_("ApplySchemeAction", automation.Action)

# Config keys
CONF_SOURCE_COLOR = "source_color"
CONF_IS_DARK = "is_dark"
CONF_CONTRAST_LEVEL = "contrast_level"
CONF_VARIANT = "variant"
CONF_ON_SCHEME_GENERATED = "on_scheme_generated"

# Scheme variants
SCHEME_VARIANTS = {
    "TONAL_SPOT": 0,
    "VIBRANT": 1,
    "EXPRESSIVE": 2,
    "CONTENT": 3,
    "MONOCHROME": 4,
    "NEUTRAL": 5,
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(MaterialThemeComponent),
    cv.Optional(CONF_SOURCE_COLOR, default=0xFF2A9D8F): cv.hex_uint32_t,
    cv.Optional(CONF_IS_DARK, default=False): cv.boolean,
    cv.Optional(CONF_CONTRAST_LEVEL, default=0.0): cv.float_range(min=-1.0, max=1.0),
    cv.Optional(CONF_VARIANT, default="TONAL_SPOT"): cv.enum(SCHEME_VARIANTS, upper=True),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    """Generate code for Material Theme component."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add(var.set_source_color(config[CONF_SOURCE_COLOR]))
    cg.add(var.set_is_dark(config[CONF_IS_DARK]))
    cg.add(var.set_contrast_level(config[CONF_CONTRAST_LEVEL]))
    cg.add(var.set_variant(config[CONF_VARIANT]))
    
    # Add Material Color Utilities C++ library include path
    # Get the component directory and add it as an include path
    # Note: All .cpp files in cpp/ subdirectories are automatically compiled by ESPHome
    component_dir = Path(__file__).parent
    cg.add_build_flag(f"-I{component_dir}")


# Action schemas
GENERATE_SCHEME_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(MaterialThemeComponent),
    cv.Optional(CONF_SOURCE_COLOR): cv.templatable(cv.hex_uint32_t),
    cv.Optional(CONF_IS_DARK): cv.templatable(cv.boolean),
    cv.Optional(CONF_CONTRAST_LEVEL): cv.templatable(cv.float_range(min=-1.0, max=1.0)),
    cv.Optional(CONF_VARIANT): cv.templatable(cv.enum(SCHEME_VARIANTS, upper=True)),
})


@automation.register_action(
    "material_theme.generate_scheme",
    GenerateSchemeAction,
    GENERATE_SCHEME_SCHEMA,
)
async def generate_scheme_to_code(config, action_id, template_arg, args):
    """Generate scheme action."""
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    
    if CONF_SOURCE_COLOR in config:
        template_ = await cg.templatable(config[CONF_SOURCE_COLOR], args, cg.uint32)
        cg.add(var.set_source_color(template_))
    
    if CONF_IS_DARK in config:
        template_ = await cg.templatable(config[CONF_IS_DARK], args, bool)
        cg.add(var.set_is_dark(template_))
    
    if CONF_CONTRAST_LEVEL in config:
        template_ = await cg.templatable(config[CONF_CONTRAST_LEVEL], args, float)
        cg.add(var.set_contrast_level(template_))
    
    if CONF_VARIANT in config:
        # For enum types, use the integer value from the enum mapping
        template_ = await cg.templatable(config[CONF_VARIANT], args, cg.int_)
        cg.add(var.set_variant(cg.RawExpression(f"static_cast<esphome::material_theme::SchemeVariant>({template_})")))
    
    return var
