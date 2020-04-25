#ifndef PROTOTYPE_2_CONFIG_H
#define PROTOTYPE_2_CONFIG_H
// This is the config file for prototype2

/* VERSION */
#define prototype2_VERSION_MAJOR 1
#define prototype2_VERSION_MINOR 0

/* PATHS */
#define RESOURCE_PATH "/Users/arnestenkrona/Documents/repositories/prototype2/res/"

/* MEMORY */
/* Size of the entity manager stack in bytes
 * The stack is used for the entity container
 * and the component managers
 */
#define ENTITY_MANAGER_STACK_SIZE_BYTES 1*1024*1024
#define DEFAULT_CONTAINER_ALLOCATOR_SIZE_BYTES 64*1024*1024
#define DEFAULT_CONTAINER_ALLOCATOR_BLOCK_SIZE_BYTES 256
#define DEFAULT_CONTAINER_ALLOCATOR_ALIGNMENT_BYTES 4

/* ENTITY COMPONENT SYSTEM */
#define TOTAL_ENTITIES 16384

/* ENTITIES */
#define MAXIMUM_MODEL_ENTITIES 10

/* GRAPHICS */
#define NUMBER_SUPPORTED_TEXTURES 20
#define NUMBER_SUPPORTED_MODEL_MATRICES 100 
#define NUMBER_SUPPORTED_POINTLIGHTS 4

/* GAME */
#define FRAME_RATE 60

#endif
