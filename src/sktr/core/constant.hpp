#pragma once

#include "sktr/pch.hpp"

constexpr uint32_t WindowWidth = 800;
constexpr uint32_t WindowHeight = 600;
const std::vector<const char*> ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

// const std::string MODEL_PATH = "models/CYG.obj";
const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "resources/viking_room.png";

#ifdef NDEBUG
const bool EnableValidationLayers = false;
#else
const bool EnableValidationLayers = true;
#endif