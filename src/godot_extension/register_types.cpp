#include <godot_cpp/godot.hpp>
#include <gdextension_interface.h>
#include "FallingSandSimulation.h"
#include "GridRenderer.h"

using namespace godot;

void initialize_falling_sand_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // Register the FallingSandSimulation class
    ClassDB::register_class<FallingSandSimulation>();
    ClassDB::register_class<GridRenderer>();
}

void uninitialize_falling_sand_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

// GDExtension entry point
extern "C" {

GDExtensionBool GDE_EXPORT godot_falling_sand_gdextension_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_falling_sand_module);
    init_obj.register_terminator(uninitialize_falling_sand_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}

} // extern "C"
