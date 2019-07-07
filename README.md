# Prototype 2
Prototype 2 is a simple game engine. It is a successor to [Prototype](https://github.com/ArneStenkrona/Prototype).

## Dependencies
Vulkan

## Setting up Vulkan
* Download [VulkanSDK](https://vulkan.lunarg.com/sdk/home)

### Linux/Windows
As far as I know, it is sufficient that the Vulkan SDK is installed.

### MacOS
MacOS does not actually natively support Vulkan. However KhronosGroup has provided MoltenVK, an implementation of Vulkan that translates Vulkan calls to Metal, Apple's graphics API. As such, we require a bit of extra work to get Vulkan working on Mac.

* Make sure you've downloaded the MacOS version of the VulkanSDK
* Add Vulkan to your environment variables.
```
 $ echo "export VK_ICD_FILENAMES=[YOUR PATH TO VULKAN HERE]/macOS/etc/vulkan/icd.d/MoltenVK_icd.json" >> ~/.profile
```
```
 $ echo "export VULKAN_SDK=[YOUR PATH TO VULKAN HERE]/macOS" >> ~/.profile
```

## Building

* Make sure you are in the root directory
```
$ cd [YOUR PATH TO prototype2]
```

* Then run the following
```
$ cmake -H. -Bbuild
```
```
$ cmake --build build -- -j3
```
## Authors

* **Arne Stenkrona**
