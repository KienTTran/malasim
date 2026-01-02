#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <torch/script.h>
#include <string>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>

class ModelLoader {
public:
    static torch::jit::Module load_model(const std::string& model_path) {
        try {
            torch::jit::Module model = torch::jit::load(model_path);
            spdlog::info("Model loaded successfully from {}", model_path);
            return model;
        } catch (const c10::Error& e) {
            spdlog::error("Error loading the model from {}: {}", model_path, e.what());
            throw; // Re-throw to let caller handle
        }
    }

    static YAML::Node load_manifest(const std::string& manifest_path) {
        try {
            YAML::Node manifest = YAML::LoadFile(manifest_path);
            spdlog::info("Manifest loaded successfully from {}", manifest_path);
            return manifest;
        } catch (const std::exception& e) {
            spdlog::error("Error loading the manifest from {}: {}", manifest_path, e.what());
            throw;
        }
    }
};

#endif // MODELLOADER_H
